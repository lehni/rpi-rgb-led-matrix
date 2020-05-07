const { spawn } = require('child_process')

// enum RGBLedCommand
const LED_COMMAND_SET_BRIGHTNESS = 1
const LED_COMMAND_SET_GAMMA = 2
const LED_COMMAND_CLEAR = 3
const LED_COMMAND_FILL = 4
const LED_COMMAND_SET_PIXELS = 5

class RGBLedMatrix {
  constructor(options = {}) {
    const args = convertOptionsToArguments(options)
    // For better handling of all the delicate timing issue, we spawn the
    // LED interface in an separate process, and just pipe data to it.
    this.interface = spawn('./rgb-led-interface', args, {
      cwd: __dirname,
      stdio: ['pipe', 'inherit', 'inherit']
    })
    this.interface.on('close', code => process.exit(code))

    process.on('exit', () => this.terminate())
    process.on('uncaughtException', err => {
      console.error(err)
      if (this.terminate()) {
        process.exit(1)
      }
    })
  }

  terminate() {
    if (this.interface) {
      this.interface.kill()
      this.interface = null
      return true
    }
    return false
  }

  writeBuffer(buffer) {
    return this.interface.stdin.write(buffer)
  }

  writeCommand(command, ...bytes) {
    return this.writeBuffer(Buffer.from([command, ...bytes]))
  }

  setBrightness(brightness) {
    return this.writeCommand(LED_COMMAND_SET_BRIGHTNESS, brightness)
  }

  setGamma(gamma) {
    const buffer = Buffer.alloc(5)
    buffer.writeUInt8(LED_COMMAND_SET_GAMMA, 0)
    buffer.writeFloatLE(gamma, 1)
    return this.writeBuffer(buffer)
  }

  clear() {
    return this.writeCommand(LED_COMMAND_CLEAR)
  }

  fill(r, g, b) {
    return this.writeCommand(LED_COMMAND_FILL, r, g, b)
  }

  setPixels(imageData) {
    const { width, height, data } = imageData
    const bytesPerPixel = data.length / (width * height)
    const buffer = Buffer.alloc(6)
    buffer.writeUInt8(LED_COMMAND_SET_PIXELS, 0)
    buffer.writeUInt16LE(width, 1)
    buffer.writeUInt16LE(height, 3)
    buffer.writeUInt8(bytesPerPixel, 5)
    return this.writeBuffer(Buffer.concat([buffer, Buffer.from(data.buffer)]))
  }
}

function convertOptionsToArguments(options) {
  // Convert object base camelized key-value configurations to hyphenated
  // command line arguments
  return Object.entries(options).reduce(
    (args, [key, value]) => [
      ...args,
      typeof value === 'boolean'
        ? value
          ? `--led-${decamelize(key)}`
          : null
        : `--led-${decamelize(key)}=${value}`
    ],
    []
  ).filter(arg => arg)
}

function decamelize(str, sep = '-') {
  return str
    ? str
      .replace(
        /([a-z\d])([A-Z])|(\S)(?:\s+)(\S)/g,
        (all, lower, upper, beforeSpace, afterSpace) => upper
          ? `${lower}${sep}${upper}`
          : `${beforeSpace}${sep}${afterSpace}`
      )
      .toLowerCase()
    : ''
}

module.exports = RGBLedMatrix
