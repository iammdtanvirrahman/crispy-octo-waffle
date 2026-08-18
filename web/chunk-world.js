import * as THREE from 'three';

export const CHUNK_SIZE = 16;
export const CHUNK_HEIGHT = 64;
const VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT;

export const BLOCKS = [
  { id:'air', color:0x000000 },
  { id:'grass', color:0x63a843 },
  { id:'dirt', color:0x8a5a36 },
  { id:'stone', color:0x7e858d },
  { id:'wood', color:0x9b6b3c },
  { id:'sand', color:0xd9c47f },
  { id:'leaves', color:0x3f8c4d }
];

const blockIndex = Object.fromEntries(BLOCKS.map((b,i)=>[b.id,i]));
const cubeFaces = [
  { n:[ 1,0,0], v:[[1,0,0],[1,1,0],[1,1,1],[1,0,1]] },
  { n:[-1,0,0], v:[[0,0,1],[0,1,1],[0,1,0],[0,0,0]] },
  { n:[0,1,0], v:[[0,1,1],[1,1,1],[1,1,0],[0,1,0]] },
  { n:[0,-1,0], v:[[0,0,0],[1,0,0],[1,0,1],[0,0,1]] },
  { n:[0,0,1], v:[[1,0,1],[1,1,1],[0,1,1],[0,0,1]] },
  { n:[0,0,-1],v:[[0,0,0],[0,1,0],[1,1,0],[1,0,0]] }
];

const idx = (x,y,z)=>y*CHUNK_SIZE*CHUNK_SIZE+z*CHUNK_SIZE+x;
const wrap = n => ((n % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

export class Chunk {
  constructor(cx,cz){
    this.cx=cx; this.cz=cz;
    this.data=new Uint8Array(VOLUME);
    this.mesh=null;
    this.dirty=true;
  }
  get(x,y,z){
    if(y<0||y>=CHUNK_HEIGHT) return 0;
    if(x<0||x>=CHUNK_SIZE||z<0||z>=CHUNK_SIZE) return 0;
    return this.data[idx(x,y,z)];
  }
  set(x,y,z,b){
    if(x<0||x>=CHUNK_SIZE||y<0||y>=CHUNK_HEIGHT||z<0||z>=CHUNK_SIZE) return;
    this.data[idx(x,y,z)]=b; this.dirty=true;
  }
}

export class ChunkWorld {
  constructor(scene, seed=1337){
    this.scene=scene;
    this.seed=seed>>>0;
    this.chunks=new Map();
    this.renderRadius=4;
    this.materials={};
    for(let i=1;i<BLOCKS.length;i++) this.materials[i]=new THREE.MeshLambertMaterial({color:BLOCKS[i].color});
  }

  key(cx,cz){return `${cx}|${cz}`;}

  hash2(x,z){
    let h=(this.seed ^ Math.imul(x,0x45d9f3b) ^ Math.imul(z,0x119de1f3))>>>0;
    h=Math.imul(h^(h>>>16),0x27d4eb2d)>>>0;
    h=Math.imul(h^(h>>>15),0x85ebca6b)>>>0;
    return ((h^(h>>>13))>>>0)/4294967295;
  }

  height(wx,wz){
    const broad=this.hash2(Math.floor(wx/32),Math.floor(wz/32));
    const mid=this.hash2(Math.floor(wx/8),Math.floor(wz/8));
    const fine=this.hash2(wx,wz);
    return Math.max(2,Math.min(CHUNK_HEIGHT-2,Math.floor(10+broad*16+mid*7+fine*3)));
  }

  generateChunk(cx,cz){
    const chunk=new Chunk(cx,cz);
    for(let z=0;z<CHUNK_SIZE;z++) for(let x=0;x<CHUNK_SIZE;x++){
      const wx=cx*CHUNK_SIZE+x, wz=cz*CHUNK_SIZE+z;
      const h=this.height(wx,wz);
      for(let y=0;y<=h;y++){
        let b=blockIndex.stone;
        if(y===h) b=h<=7?blockIndex.sand:blockIndex.grass;
        else if(y>=h-3) b=blockIndex.dirt;
        chunk.set(x,y,z,b);
      }
      const treeChance=this.hash2(wx+91,wz-37);
      if(treeChance>0.975 && h>8 && x>2 && x<13 && z>2 && z<13){
        for(let y=1;y<=4&&h+y<CHUNK_HEIGHT;y++) chunk.set(x,h+y,z,blockIndex.wood);
        for(let dx=-2;dx<=2;dx++) for(let dz=-2;dz<=2;dz++){
          if(Math.abs(dx)+Math.abs(dz)<4) chunk.set(x+dx,h+4,z+dz,blockIndex.leaves);
        }
      }
    }
    this.chunks.set(this.key(cx,cz),chunk);
    return chunk;
  }

  ensureChunk(cx,cz){return this.chunks.get(this.key(cx,cz))||this.generateChunk(cx,cz);}

  getBlock(wx,wy,wz){
    if(wy<0||wy>=CHUNK_HEIGHT) return 0;
    const cx=Math.floor(wx/CHUNK_SIZE),cz=Math.floor(wz/CHUNK_SIZE);
    const chunk=this.ensureChunk(cx,cz);
    return chunk.get(wrap(wx),wy,wrap(wz));
  }

  setBlock(wx,wy,wz,b){
    if(wy<0||wy>=CHUNK_HEIGHT) return false;
    const cx=Math.floor(wx/CHUNK_SIZE),cz=Math.floor(wz/CHUNK_SIZE);
    const chunk=this.ensureChunk(cx,cz);
    const lx=wrap(wx),lz=wrap(wz);
    chunk.set(lx,wy,lz,b);
    this.rebuildChunk(cx,cz);
    if(lx===0) this.rebuildChunk(cx-1,cz);
    if(lx===CHUNK_SIZE-1) this.rebuildChunk(cx+1,cz);
    if(lz===0) this.rebuildChunk(cx,cz-1);
    if(lz===CHUNK_SIZE-1) this.rebuildChunk(cx,cz+1);
    return true;
  }

  rebuildChunk(cx,cz){
    const chunk=this.ensureChunk(cx,cz);
    if(chunk.mesh){this.scene.remove(chunk.mesh);chunk.mesh.geometry.dispose();chunk.mesh=null;}
    const positions=[],normals=[],colors=[],indices=[];
    for(let y=0;y<CHUNK_HEIGHT;y++) for(let z=0;z<CHUNK_SIZE;z++) for(let x=0;x<CHUNK_SIZE;x++){
      const b=chunk.get(x,y,z); if(!b) continue;
      const wx=cx*CHUNK_SIZE+x,wz=cz*CHUNK_SIZE+z;
      for(const face of cubeFaces){
        const nb=this.getBlock(wx+face.n[0],y+face.n[1],wz+face.n[2]);
        if(nb!==0) continue;
        const base=positions.length/3;
        for(const p of face.v){positions.push(wx+p[0],y+p[1],wz+p[2]);normals.push(...face.n);const c=new THREE.Color(BLOCKS[b].color);colors.push(c.r,c.g,c.b);}
        indices.push(base,base+1,base+2,base,base+2,base+3);
      }
    }
    const g=new THREE.BufferGeometry();
    g.setAttribute('position',new THREE.Float32BufferAttribute(positions,3));
    g.setAttribute('normal',new THREE.Float32BufferAttribute(normals,3));
    g.setAttribute('color',new THREE.Float32BufferAttribute(colors,3));
    g.setIndex(indices);
    const mat=new THREE.MeshLambertMaterial({vertexColors:true});
    chunk.mesh=new THREE.Mesh(g,mat);
    chunk.mesh.userData={cx,cz};
    this.scene.add(chunk.mesh);
    chunk.dirty=false;
  }

  stream(playerX,playerZ){
    const pcx=Math.floor(playerX/CHUNK_SIZE),pcz=Math.floor(playerZ/CHUNK_SIZE);
    const wanted=new Set();
    for(let dz=-this.renderRadius;dz<=this.renderRadius;dz++) for(let dx=-this.renderRadius;dx<=this.renderRadius;dx++){
      if(dx*dx+dz*dz>this.renderRadius*this.renderRadius) continue;
      const cx=pcx+dx,cz=pcz+dz,k=this.key(cx,cz);wanted.add(k);
      const chunk=this.ensureChunk(cx,cz);
      if(chunk.dirty) this.rebuildChunk(cx,cz);
    }
    for(const [k,ch] of this.chunks){
      if(!wanted.has(k)){if(ch.mesh){this.scene.remove(ch.mesh);ch.mesh.geometry.dispose();ch.mesh.material.dispose();ch.mesh=null;}this.chunks.delete(k);}
    }
  }
}
