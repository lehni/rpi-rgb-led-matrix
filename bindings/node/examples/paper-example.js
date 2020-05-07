const paper = require('paper')
const RGBLedMatrix = require('..')

const interval = 1000 / 60
const useTimer = true

const width = 32
const height = 48

const matrix = new RGBLedMatrix([
  '--led-rows=16',
  '--led-cols=16',
  '--led-chain=6',
  '--led-gpio-mapping=adafruit-hat-pwm',
  '--led-multiplexing=10',
  '--led-pixel-mapper=U-mapper',
  '--led-pwm-lsb-nanoseconds=200',
  '--led-brightness=50'
])

paper.setup(new paper.Size(width, height))

const circle = new paper.Shape.Circle({
  center: paper.view.center,
  radius: 15,
  fillColor: 'red'
})

let i = 0
function drawFrame() {
  circle.scaling = Math.sin(i / 50) / 2 + 1 // 0.5
  i++
  paper.view.update()
  matrix.setPixels(paper.view._context.getImageData(0, 0, width, height))
}

function msleep(n) {
  Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, n);
}

matrix.setBrightness(50)
matrix.clear()

if (useTimer) {
  setInterval(drawFrame, interval)
} else {
  while (true) {
    const start = Date.now()
    drawFrame()
    const wait = start + interval - Date.now()
    if (wait > 0) {
      msleep(wait)
    }
  }
}
