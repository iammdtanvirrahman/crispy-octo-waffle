import * as THREE from 'three';

export const CHUNK_SIZE = 16;
export const CHUNK_HEIGHT = 64;
const VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT;

export const BLOCKS = [
  { id: 'air', color: 0x000000 },
  { id: 'grass', color: 0x63a843 },
  { id: 'dirt', color: 0x8a5a36 },
  { id: 'stone', color: 0x7e858d },
  { id: 'wood', color: 0x9b6b3c },
  { id: 'sand', color: 0xd9c47f },
  { id: 'leaves', color: 0x3f8c4d }
];

const blockIndex = Object.fromEntries(BLOCKS.map((b, i) => [b.id, i]));
const indexOf = (x, y, z) => y * CHUNK_SIZE * CHUNK_SIZE + z * CHUNK_SIZE + x;
const wrap = n => ((n % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
const solid = b => b !== 0;

export class Chunk {
  constructor(cx, cz) {
    this.cx = cx;
    this.cz = cz;
    this.data = new Uint8Array(VOLUME);
    this.mesh = null;
    this.dirty = true;
  }

  get(x, y, z) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) return 0;
    return this.data[indexOf(x, y, z)];
  }

  set(x, y, z, block) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) return;
    this.data[indexOf(x, y, z)] = block;
    this.dirty = true;
  }
}

export class ChunkWorld {
  constructor(scene, seed = 1337) {
    this.scene = scene;
    this.seed = seed >>> 0;
    this.chunks = new Map();
    this.renderRadius = 4;
    this.maxRebuildsPerFrame = 2;
    this.rebuildQueue = [];
    this.rebuildQueued = new Set();
  }

  key(cx, cz) {
    return `${cx}|${cz}`;
  }

  hash2(x, z) {
    let h = (this.seed ^ Math.imul(x, 0x45d9f3b) ^ Math.imul(z, 0x119de1f3)) >>> 0;
    h = Math.imul(h ^ (h >>> 16), 0x27d4eb2d) >>> 0;
    h = Math.imul(h ^ (h >>> 15), 0x85ebca6b) >>> 0;
    return ((h ^ (h >>> 13)) >>> 0) / 4294967295;
  }

  height(wx, wz) {
    const broad = this.hash2(Math.floor(wx / 32), Math.floor(wz / 32));
    const mid = this.hash2(Math.floor(wx / 8), Math.floor(wz / 8));
    const fine = this.hash2(wx, wz);
    return Math.max(2, Math.min(CHUNK_HEIGHT - 2, Math.floor(10 + broad * 16 + mid * 7 + fine * 3)));
  }

  markDirty(cx, cz) {
    const c = this.chunks.get(this.key(cx, cz));
    if (!c) return;
    c.dirty = true;
    this.enqueueRebuild(cx, cz);
  }

  generateChunk(cx, cz) {
    const c = new Chunk(cx, cz);

    for (let z = 0; z < CHUNK_SIZE; z++) {
      for (let x = 0; x < CHUNK_SIZE; x++) {
        const wx = cx * CHUNK_SIZE + x;
        const wz = cz * CHUNK_SIZE + z;
        const h = this.height(wx, wz);

        for (let y = 0; y <= h; y++) {
          let block = blockIndex.stone;
          if (y === h) block = h <= 7 ? blockIndex.sand : blockIndex.grass;
          else if (y >= h - 3) block = blockIndex.dirt;
          c.set(x, y, z, block);
        }

        const tree = this.hash2(wx + 91, wz - 37);
        if (tree > 0.975 && h > 8 && x > 2 && x < 13 && z > 2 && z < 13) {
          for (let y = 1; y <= 4 && h + y < CHUNK_HEIGHT; y++) c.set(x, h + y, z, blockIndex.wood);
          for (let dx = -2; dx <= 2; dx++) {
            for (let dz = -2; dz <= 2; dz++) {
              if (Math.abs(dx) + Math.abs(dz) < 4 && x + dx >= 0 && x + dx < CHUNK_SIZE && z + dz >= 0 && z + dz < CHUNK_SIZE) {
                c.set(x + dx, h + 4, z + dz, blockIndex.leaves);
              }
            }
          }
        }
      }
    }

    this.chunks.set(this.key(cx, cz), c);

    // Loading a neighbor changes boundary visibility of already-loaded chunks.
    this.markDirty(cx - 1, cz);
    this.markDirty(cx + 1, cz);
    this.markDirty(cx, cz - 1);
    this.markDirty(cx, cz + 1);

    return c;
  }

  ensureChunk(cx, cz) {
    return this.chunks.get(this.key(cx, cz)) || this.generateChunk(cx, cz);
  }

  peekBlock(wx, wy, wz) {
    if (wy < 0 || wy >= CHUNK_HEIGHT) return 0;
    const cx = Math.floor(wx / CHUNK_SIZE);
    const cz = Math.floor(wz / CHUNK_SIZE);
    const c = this.chunks.get(this.key(cx, cz));
    return c ? c.get(wrap(wx), wy, wrap(wz)) : 0;
  }

  getBlock(wx, wy, wz) {
    if (wy < 0 || wy >= CHUNK_HEIGHT) return 0;
    return this.ensureChunk(Math.floor(wx / CHUNK_SIZE), Math.floor(wz / CHUNK_SIZE)).get(wrap(wx), wy, wrap(wz));
  }

  enqueueRebuild(cx, cz) {
    const key = this.key(cx, cz);
    if (!this.chunks.has(key) || this.rebuildQueued.has(key)) return;
    this.rebuildQueue.push([cx, cz]);
    this.rebuildQueued.add(key);
  }

  setBlock(wx, wy, wz, block) {
    if (wy < 0 || wy >= CHUNK_HEIGHT) return false;
    const cx = Math.floor(wx / CHUNK_SIZE);
    const cz = Math.floor(wz / CHUNK_SIZE);
    const lx = wrap(wx);
    const lz = wrap(wz);
    const c = this.ensureChunk(cx, cz);
    c.set(lx, wy, lz, block);

    this.markDirty(cx, cz);
    if (lx === 0) this.markDirty(cx - 1, cz);
    if (lx === CHUNK_SIZE - 1) this.markDirty(cx + 1, cz);
    if (lz === 0) this.markDirty(cx, cz - 1);
    if (lz === CHUNK_SIZE - 1) this.markDirty(cx, cz + 1);
    return true;
  }

  appendQuad(positions, normals, colors, indices, corners, normal, block) {
    const base = positions.length / 3;
    const color = new THREE.Color(BLOCKS[block].color);
    for (const p of corners) {
      positions.push(p[0], p[1], p[2]);
      normals.push(normal[0], normal[1], normal[2]);
      colors.push(color.r, color.g, color.b);
    }
    indices.push(base, base + 1, base + 2, base, base + 2, base + 3);
  }

  rebuildChunk(cx, cz) {
    const chunk = this.ensureChunk(cx, cz);
    if (chunk.mesh) {
      this.scene.remove(chunk.mesh);
      chunk.mesh.geometry.dispose();
      chunk.mesh.material.dispose();
      chunk.mesh = null;
    }

    const positions = [];
    const normals = [];
    const colors = [];
    const indices = [];
    const dims = [CHUNK_SIZE, CHUNK_HEIGHT, CHUNK_SIZE];

    for (let axis = 0; axis < 3; axis++) {
      const uAxis = (axis + 1) % 3;
      const vAxis = (axis + 2) % 3;
      const widthSize = dims[uAxis];
      const heightSize = dims[vAxis];
      const depthSize = dims[axis];

      for (const positive of [false, true]) {
        const mask = new Int16Array(widthSize * heightSize);

        for (let slice = 0; slice <= depthSize; slice++) {
          // Adjacent voxels separated by this plane.
          for (let v = 0; v < heightSize; v++) {
            for (let u = 0; u < widthSize; u++) {
              const left = [0, 0, 0];
              const right = [0, 0, 0];
              left[axis] = slice - 1;
              right[axis] = slice;
              left[uAxis] = right[uAxis] = u;
              left[vAxis] = right[vAxis] = v;

              const leftBlock = this.peekBlock(cx * CHUNK_SIZE + left[0], left[1], cz * CHUNK_SIZE + left[2]);
              const rightBlock = this.peekBlock(cx * CHUNK_SIZE + right[0], right[1], cz * CHUNK_SIZE + right[2]);

              mask[v * widthSize + u] = positive
                ? (solid(leftBlock) && !solid(rightBlock) ? leftBlock : 0)
                : (!solid(leftBlock) && solid(rightBlock) ? rightBlock : 0);
            }
          }

          // Greedily merge identical visible faces into one rectangle.
          for (let v = 0; v < heightSize; v++) {
            for (let u = 0; u < widthSize;) {
              const block = mask[v * widthSize + u];
              if (!block) {
                u++;
                continue;
              }

              let w = 1;
              while (u + w < widthSize && mask[v * widthSize + u + w] === block) w++;

              let h = 1;
              outer: while (v + h < heightSize) {
                for (let x = 0; x < w; x++) {
                  if (mask[(v + h) * widthSize + u + x] !== block) break outer;
                }
                h++;
              }

              const plane = positive ? slice : slice - 1;
              const origin = [0, 0, 0];
              origin[axis] = plane;
              origin[uAxis] = u;
              origin[vAxis] = v;

              const du = [0, 0, 0];
              const dv = [0, 0, 0];
              du[uAxis] = w;
              dv[vAxis] = h;

              const make = (a, b) => [
                origin[0] + du[0] * a + dv[0] * b + cx * CHUNK_SIZE,
                origin[1] + du[1] * a + dv[1] * b,
                origin[2] + du[2] * a + dv[2] * b + cz * CHUNK_SIZE
              ];

              const p0 = make(0, 0);
              const p1 = make(1, 0);
              const p2 = make(1, 1);
              const p3 = make(0, 1);
              const normal = [0, 0, 0];
              normal[axis] = positive ? 1 : -1;

              this.appendQuad(
                positions,
                normals,
                colors,
                indices,
                positive ? [p0, p1, p2, p3] : [p0, p3, p2, p1],
                normal,
                block
              );

              for (let y = 0; y < h; y++) {
                for (let x = 0; x < w; x++) mask[(v + y) * widthSize + u + x] = 0;
              }
              u += w;
            }
          }
        }
      }
    }

    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
    geometry.setAttribute('normal', new THREE.Float32BufferAttribute(normals, 3));
    geometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
    geometry.setIndex(indices);
    geometry.computeBoundingSphere();

    chunk.mesh = new THREE.Mesh(geometry, new THREE.MeshLambertMaterial({ vertexColors: true }));
    chunk.mesh.userData = { cx, cz };
    this.scene.add(chunk.mesh);
    chunk.dirty = false;
  }

  processRebuildQueue() {
    let processed = 0;
    while (this.rebuildQueue.length && processed < this.maxRebuildsPerFrame) {
      const [cx, cz] = this.rebuildQueue.shift();
      this.rebuildQueued.delete(this.key(cx, cz));
      if (this.chunks.has(this.key(cx, cz))) this.rebuildChunk(cx, cz);
      processed++;
    }
  }

  stream(px, pz) {
    const pcx = Math.floor(px / CHUNK_SIZE);
    const pcz = Math.floor(pz / CHUNK_SIZE);
    const wanted = [];

    for (let dz = -this.renderRadius; dz <= this.renderRadius; dz++) {
      for (let dx = -this.renderRadius; dx <= this.renderRadius; dx++) {
        if (dx * dx + dz * dz > this.renderRadius * this.renderRadius) continue;
        const cx = pcx + dx;
        const cz = pcz + dz;
        wanted.push([cx, cz]);
        const c = this.ensureChunk(cx, cz);
        if (c.dirty) this.enqueueRebuild(cx, cz);
      }
    }

    const keep = new Set(wanted.map(([x, z]) => this.key(x, z)));
    for (const [key, c] of this.chunks) {
      if (!keep.has(key)) {
        if (c.mesh) {
          this.scene.remove(c.mesh);
          c.mesh.geometry.dispose();
          c.mesh.material.dispose();
        }
        this.chunks.delete(key);
        this.rebuildQueued.delete(key);
      }
    }

    this.processRebuildQueue();
  }
}
