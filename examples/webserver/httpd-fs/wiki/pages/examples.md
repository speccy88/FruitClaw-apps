# Examples

These examples are copy-paste starting points. Replace `DEVICE_IP` with the
board's address from `ifconfig wlan0`.

## First Shell Session

```sh
help
uname -a
ifconfig wlan0
fruitclaw status
fruitclaw tools
```

`help` lists NuttX shell commands and built-in apps. `fruitclaw status` is the
best first health snapshot.

## Web And MCP Smoke Test

```sh
curl http://DEVICE_IP/
curl -i http://DEVICE_IP/mcp \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"ping"}'
```

List tools:

```sh
curl -i http://DEVICE_IP/mcp \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}'
```

Call a safe tool:

```sh
curl -i http://DEVICE_IP/mcp \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"time.now","arguments":{}}}'
```

Get a feature summary:

```sh
curl -i http://DEVICE_IP/mcp \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"system.info","arguments":{}}}'
```

## Telegram Bring-Up

```sh
fruitclaw telegram-discover
fruitclaw telegram-test
fruitclaw telegram-inject "what time is it? use the time tool"
```

Allowed chat IDs live in:

```text
/data/fruitclaw/telegram_allowed_chats.txt
```

Telegram inbound handling is text-only in this build.

## One-Shot Agent Prompt

```sh
fruitclaw once "what time is it? use the time tool"
```

This exercises the local event queue, router, agent worker, DeepSeek call,
tool call, session logging, and reply path without needing a live Telegram
update.

## NeoPixels

```sh
fruitclaw neopixels blue
fruitclaw neopixels 255 0 0
fruitclaw neopixels rainbow blue 2
fruitclaw neopixels chase green 3
fruitclaw neopixels pulse purple 2
fruitclaw neopixels off
```

MCP tool names:

```text
neopixels.set
neopixels.effect
neopixels.off
```

## DVI And Game Of Life

```sh
dvictrl start
dvictrl info
cgol
```

`cgol` runs Conway's Game of Life on `/dev/fb0` and stays in the foreground.
Press Ctrl-C to stop it.

## Terminal And Devices

Safe terminal examples:

```sh
fruitclaw terminal-run uname -a
fruitclaw terminal-run help
```

Prefer dedicated device tools for `/dev`:

```sh
fruitclaw device list
fruitclaw device read /dev/leds0 16
```

`device.read` and `device.write` are for bounded non-block device paths. They
reject raw SD/media paths such as `/dev/mmcsd0`.

MCP examples:

```json
{"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"device.list","arguments":{}}}
```

```json
{"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"terminal.run","arguments":{"command":"uname -a"}}}
```

## Schedules

```sh
fruitclaw schedule list
fruitclaw schedule add-after reminder 60 "Send a reminder"
fruitclaw schedule add-interval heartbeat 3600 "Run heartbeat"
fruitclaw schedule add-cron morning "0 8 * * *" "Run morning workflow"
fruitclaw schedule remove reminder
```

Plain schedule prompts are delivered directly at fire time. Prefix the prompt
with `agent:` only when the scheduled job should wake DeepSeek and use tools.

## Berry

Berry scripts are resolved under the active data-root `scripts/` directory.

Example script:

```be
import claw
claw.reply(claw.terminal_run("uname -a"))
```

Run:

```sh
fruitclaw berry-smoke
fruitclaw berry-run example.be '{}'
```

The constrained `claw` module includes `args()`, `reply()`, `tool()`,
`memory_append()`, `terminal_run()`, `neopixels_set()`, and `schedule_add()`.

Sources: `apps/system/fruitclaw/README.md`,
`apps/system/fruitclaw/fruitclaw_main.c`,
`apps/system/fruitclaw/caps/fc_caps_builtin.c`.
