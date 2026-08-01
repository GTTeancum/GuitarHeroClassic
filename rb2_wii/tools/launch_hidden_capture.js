"use strict";

// Windows PowerShell's Start-Process cannot launch in the Codex environment
// because its inherited environment contains both PATH and Path. Node can
// preserve that environment block and detach the native renderer cleanly.

const fs = require("fs");
const { spawn } = require("child_process");

if (process.argv.length < 7) {
  console.error(
    "usage: node launch_hidden_capture.js <exe> <cwd> <stdout> <stderr> -- <args...>",
  );
  process.exit(2);
}

const separator = process.argv.indexOf("--", 6);
if (separator < 0) {
  console.error("missing -- argument separator");
  process.exit(2);
}

const exe = process.argv[2];
const cwd = process.argv[3];
const stdoutPath = process.argv[4];
const stderrPath = process.argv[5];
const childArgs = process.argv.slice(separator + 1);
const stdout = fs.openSync(stdoutPath, "w");
const stderr = fs.openSync(stderrPath, "w");
const child = spawn(exe, childArgs, {
  cwd,
  detached: true,
  windowsHide: true,
  stdio: ["ignore", stdout, stderr],
  env: { ...process.env, GHOGX_HIDE_WINDOW: "1" },
});
child.unref();
fs.closeSync(stdout);
fs.closeSync(stderr);
console.log(`pid=${child.pid}`);
