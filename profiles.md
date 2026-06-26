---
layout: page
title: Profiles
permalink: /profiles/
---

TransparentBLE profiles are JSON files that describe how a BLE device should be matched, read, parsed, and controlled.

A profile can define device name prefixes, service and characteristic UUIDs, polling actions, response fields, display order, and optional controls. This lets a user build an iOS interface for a BLE device without writing Swift.

## Available Profiles

### EB3A

Included with the app as a bundled default profile.

[Download EB3A profile](/transparentble-app/downloads/profiles/bluetti-eb3a.bleprofile.json)

### ESP32-C6 LED

A sample profile for a custom ESP32-C6 BLE LED device. It demonstrates reading state, toggling power, setting RGB values, and changing brightness.

[Download ESP32-C6 LED profile](/transparentble-app/downloads/profiles/esp32-c6-led.bleprofile.json)

## Importing Profiles

Download a `.bleprofile.json` file, then import it from TransparentBLE Settings using Device Profiles.

Imported profiles are stored locally on your device. If an imported profile uses the same profile id as a bundled profile and has a higher version, the imported profile can override the bundled one.

## Creating Profiles

To create a profile for your own BLE device, start from a working protocol description. You need to know how the device advertises itself, which GATT services and characteristics it uses, what bytes must be written, and how response bytes should be decoded.

Profiles use `schemaVersion` `1` and should be saved as `.bleprofile.json`.

## Profile Identity

Every profile needs these top-level fields:

- `schemaVersion`: currently `1`
- `id`: a stable unique id, such as `vendor.device` or `sample.board.feature`
- `name`: the device or profile name shown in the app
- `vendor`: the maker, project, or profile author name
- `version`: a version string for this profile

If an imported profile has the same `id` as a bundled profile, TransparentBLE keeps the newer version.

## Device Matching

The `match` section tells TransparentBLE when a profile may apply to a discovered BLE device.

- `namePrefixes`: advertised device-name prefixes to match
- `serviceUUIDs`: advertised or discovered service UUIDs to match

Use the most specific values you can. A broad match can make a profile appear for unrelated BLE devices.

## Services

The `services` section describes the GATT services and characteristics used by the profile.

Each service has:

- `id`: local id used by actions, such as `main`
- `uuid`: BLE service UUID
- `write`: characteristic UUIDs that can receive commands
- `notify`: characteristic UUIDs that return notifications or responses
- `read`: characteristic UUIDs that can be read directly

Actions must reference a service id from this list, and their write/notify characteristics must belong to that service.

## Actions

Actions are the requests TransparentBLE can send to the device.

Each action has:

- `id`: stable action id
- `title`: label shown in the app
- `service`: service id from `services`
- `writeCharacteristic`: characteristic used for the outgoing payload
- `notifyCharacteristic`: characteristic expected to deliver the response, when needed
- `writeType`: `withResponse` or `withoutResponse`
- `payload`: hex bytes to write
- `payloadTemplate`: optional hex template with placeholders
- `expectedResponseLength`: full response length in bytes
- `checksum`: `none` or `modbusCRC`
- `responseFieldsAction`: optional action id whose field definitions should parse this response

Payloads are written as hex bytes, for example `01 03 00 0A`. For dynamic controls, `payloadTemplate` can contain `{value}`, `{red}`, `{green}`, and `{blue}` placeholders. TransparentBLE replaces these placeholders with one-byte hex values before writing.

## Polling

The optional `polling` section defines the default refresh behavior.

- `defaultIntervalSeconds`: suggested refresh interval
- `snapshotActions`: action ids to run when refreshing a profile snapshot

Use snapshot actions for safe read-only requests that update the visible state of the device.

## Fields

Fields describe how response bytes become readable values.

Each field has:

- `id`: stable field id
- `title`: label shown in the app
- `action`: action id whose response contains this field
- `byteOffset`: zero-based start position inside the parsed response data
- `byteLength`: number of bytes to read
- `type`: parser type
- `endian`: `big` or `little`, when relevant
- `scale`: decimal scale, when using `decimal`
- `unit`: optional display unit
- `values`: value labels, when using `enum`

Supported field types:

- `uint8`, `uint16`, `uint32`
- `int8`, `int16`, `int32`
- `decimal`
- `bool`
- `enum`
- `string`
- `hex`
- `raw`
- `version`
- `serialNumber`

For `checksum: none`, field offsets are based on the full response bytes. For `checksum: modbusCRC`, TransparentBLE validates the Modbus response and parses fields from the data section only, excluding address, function, byte count, and CRC bytes.

## Display Order

The optional `ui` section controls the order of decoded fields.

- `fieldOrder`: array of field ids

Fields not listed in `fieldOrder` can still appear, but listed fields are shown first in the order you provide.

## Controls

Controls turn actions into UI elements.

Supported control types:

- `toggle`
- `valueSlider`
- `rgbSliders`

Common control fields:

- `id`: stable control id
- `title`: label shown in the app
- `type`: one of the supported control types
- `stateField`: field used to show current state
- `onAction` and `offAction`: actions for toggle controls
- `action`: action used by sliders or RGB controls
- `valueField`: field that stores the current slider value
- `valueKey`: placeholder name for `payloadTemplate`, usually `value`
- `minValue` and `maxValue`: slider range
- `unit`: optional display unit
- `redField`, `greenField`, `blueField`: fields used by RGB controls

Controls should only point to actions you understand and consider safe. If a device command can change hardware state, power output, calibration, firmware settings, or stored configuration, treat it as a control action and test carefully.

## Validation Rules

TransparentBLE validates imported profiles before using them. A profile can be rejected when:

- `schemaVersion` is unsupported
- required ids or names are empty
- service ids, action ids, or field ids are duplicated
- UUIDs are invalid
- an action references a missing service or characteristic
- an action payload is empty or invalid hex
- `expectedResponseLength` is not greater than zero
- a field references a missing action
- a field byte range falls outside the action response data
- a control references a missing field or action
- a slider has `minValue` greater than `maxValue`

## Recommended Workflow

Build a profile in small steps:

1. Add identity, matching, and services.
2. Add one safe read action.
3. Add fields for that response.
4. Import the profile and confirm the decoded values.
5. Add polling after reads work reliably.
6. Add controls only after command payloads are understood and tested.
7. Increase the profile `version` when publishing an update.

Keep a backup copy of working profiles before editing. A profile is just configuration, but it can still send real BLE commands.

Only import or share profiles you understand and trust. Profile actions can send commands to a BLE device.
