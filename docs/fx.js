// PixiJS backdrop: a sine slammed into clip rails. Saturation, not a maw. No CDN.
const C = { mint: 0x0F6B4E, sage: 0xA8C5B0, forest: 0x1A3C32, ink: 0x900020, white: 0xFFFFFF };
const BEAT = 1.28;          // seconds per slam; decay is the rest of the bar
const CYCLES = 2.4;         // visible sine periods across the viewport
const STEP = 5;             // px between samples
const TAU = Math.PI * 2;
const reduceMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;

const fxRaw = new URLSearchParams(location.search).get('fx');
const fxOnly = fxRaw !== null && !/^(0|false|off)$/i.test(fxRaw);
if (fxOnly) document.body.classList.add('fx-only');

const pointer = { x: 0.5, y: 0.5 };
if (!fxOnly) {
  addEventListener('pointermove', (e) => {
    pointer.x = e.clientX / innerWidth;
    pointer.y = e.clientY / innerHeight;
  }, { passive: true });
}

start().catch((err) => {
  console.info('fx: animated backdrop disabled:', err?.message || err);
  document.getElementById('fx')?.remove();
});

async function start() {
  const PIXI = await import('./vendor/pixi.min.mjs');
  const app = new PIXI.Application();
  await app.init({
    canvas: document.getElementById('fx'),
    resizeTo: window,
    backgroundAlpha: 0,
    antialias: true,
    autoDensity: true,
    resolution: Math.min(devicePixelRatio || 1, 2),
    preference: 'webgl',
  });

  const glow = glowTexture(PIXI);
  const scene = satScene(PIXI, glow);
  app.stage.addChild(scene.root);
  bootTicker(app, scene);
}

function bootTicker(app, scene) {
  if (reduceMotion) {
    app.ticker.stop();
    const still = () => {
      const { width: w, height: h } = app.screen;
      scene.update(BEAT * 0.08, w, h);
      app.render();
    };
    addEventListener('resize', () => requestAnimationFrame(still));
    still();
    return;
  }

  let t = 0;
  app.ticker.add((ticker) => {
    const dt = Math.min(ticker.deltaMS / 1000, 0.05);
    t += dt;
    const { width: w, height: h } = app.screen;
    if (!fxOnly) {
      const ease = Math.min(1, dt * 2.5);
      scene.root.x += ((0.5 - pointer.x) * 40 - scene.root.x) * ease;
      scene.root.y += ((0.5 - pointer.y) * 24 - scene.root.y) * ease;
    }
    scene.update(t, w, h);
  });

  document.addEventListener('visibilitychange', () => {
    if (document.hidden) app.ticker.stop(); else app.ticker.start();
  });
}

function glowTexture(PIXI, size = 64) {
  const c = document.createElement('canvas');
  c.width = c.height = size;
  const g = c.getContext('2d');
  const r = size / 2;
  const grd = g.createRadialGradient(r, r, 0, r, r, r);
  grd.addColorStop(0, 'rgba(255,255,255,1)');
  grd.addColorStop(0.25, 'rgba(255,255,255,0.5)');
  grd.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = grd;
  g.fillRect(0, 0, size, size);
  return PIXI.Texture.from(c);
}

function glowLine(g, pts, color, scale = 1, passes) {
  for (const [width, alpha] of passes) {
    g.moveTo(pts[0], pts[1]);
    for (let i = 2; i < pts.length; i += 2) g.lineTo(pts[i], pts[i + 1]);
    g.stroke({ width: width * scale, color, alpha, cap: 'round', join: 'round' });
  }
}

const DRY_PASSES = [[14, 0.04], [3, 0.1]];
const WET_PASSES = [[56, 0.05], [28, 0.1], [12, 0.22], [4, 0.45]];
const GHOST_PASSES = [[40, 0.06], [10, 0.14]];
const RAIL_PASSES = [[28, 0.05], [8, 0.12]];
const HARM_PASSES = [[18, 0.06], [4, 0.12]];

// Drive 0..1: sine stays round, then tanh-clips against the rails, harmonics leak.
function satScene(PIXI, glow) {
  const root = new PIXI.Container();
  const g = new PIXI.Graphics();
  g.blendMode = 'add';

  const bloomN = glowSprite(PIXI, glow, C.white);
  const bloomS = glowSprite(PIXI, glow, C.mint);
  bloomN.alpha = bloomS.alpha = 0;

  root.addChild(g, bloomN, bloomS);
  root.alpha = fxOnly ? 1 : 0.62;

  return {
    root,
    update(t, w, h) {
      const cy = h * (fxOnly ? 0.5 : 0.46);
      const amp = Math.min(h * 0.28, 160);
      // Hit, then die: a bar, not a swell.
      const env = Math.exp(-((t % BEAT) / BEAT) * 4.8);
      const drive = 0.22 + 0.78 * env;
      const k = 0.6 + drive * 9;
      const norm = Math.tanh(k);
      const phase = t * 1.7;
      const fuzz = 10 + 22 * drive;

      const dry = [];
      const wet = [];
      const ghost = [];
      const h3 = [];
      const h5 = [];
      for (let x = -20; x <= w + 20; x += STEP) {
        const th = (x / w) * CYCLES * TAU + phase;
        const s = Math.sin(th);
        const n = Math.sin(x * 0.21 + t * 13) * 0.55 + Math.sin(x * 0.53 - t * 7) * 0.45;
        const yDry = cy - s * amp;
        const shaped = (Math.tanh(k * s) / norm) * amp;
        const yWet = cy - shaped + n * fuzz;
        dry.push(x, yDry);
        wet.push(x, yWet);
        ghost.push(x, yWet + n * fuzz * 0.7);
        h3.push(x, cy - Math.sin(th * 3 + t * 2) * amp * 0.28 * drive + n * fuzz * 0.4);
        h5.push(x, cy - Math.sin(th * 5 - t * 1.4) * amp * 0.16 * drive - n * fuzz * 0.3);
      }

      g.clear();

      const railY = [cy - amp, cy + amp];
      for (const y of railY) {
        glowLine(g, [-40, y, w + 40, y], C.white, 1, RAIL_PASSES);
      }

      glowLine(g, dry, C.sage, 1, DRY_PASSES);
      glowLine(g, h3, C.forest, 1.4, HARM_PASSES);
      glowLine(g, h5, C.mint, 1.2, HARM_PASSES);
      glowLine(g, ghost, C.sage, 1.2, GHOST_PASSES);
      glowLine(g, wet, C.mint, 1 + drive * 0.6, WET_PASSES);

      const bloom = 0.12 + env * 0.55;
      const sx = w / 28;
      const sy = 18 + env * 36;
      bloomN.x = bloomS.x = w * 0.5;
      bloomN.y = cy - amp;
      bloomS.y = cy + amp;
      bloomN.scale.set(sx, sy);
      bloomS.scale.set(sx, sy);
      bloomN.alpha = bloom;
      bloomS.alpha = bloom * 0.7;
    },
  };
}

function glowSprite(PIXI, tex, tint) {
  const s = new PIXI.Sprite(tex);
  s.anchor.set(0.5);
  s.tint = tint;
  s.blendMode = 'add';
  return s;
}
