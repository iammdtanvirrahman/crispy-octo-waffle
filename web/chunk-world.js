import * as THREE from 'three';

export const CHUNK_SIZE = 16;
export const CHUNK_HEIGHT = 64;
const VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT;
export const BLOCKS = [
  {id:'air',color:0x000000},{id:'grass',color:0x63a843},{id:'dirt',color:0x8a5a36},
  {id:'stone',color:0x7e858d},{id:'wood',color:0x9b6b3c},{id:'sand',color:0xd9c47f},{id:'leaves',color:0x3f8c4d}
];
const blockIndex=Object.fromEntries(BLOCKS.map((b,i)=>[b.id,i]));
const faces=[
 {n:[1,0,0],v:[[1,0,0],[1,1,0],[1,1,1],[1,0,1]]},{n:[-1,0,0],v:[[0,0,1],[0,1,1],[0,1,0],[0,0,0]]},
 {n:[0,1,0],v:[[0,1,1],[1,1,1],[1,1,0],[0,1,0]]},{n:[0,-1,0],v:[[0,0,0],[1,0,0],[1,0,1],[0,0,1]]},
 {n:[0,0,1],v:[[1,0,1],[1,1,1],[0,1,1],[0,0,1]]},{n:[0,0,-1],v:[[0,0,0],[0,1,0],[1,1,0],[1,0,0]]}
];
const idx=(x,y,z)=>y*CHUNK_SIZE*CHUNK_SIZE+z*CHUNK_SIZE+x;
const wrap=n=>((n%CHUNK_SIZE)+CHUNK_SIZE)%CHUNK_SIZE;

export class Chunk{
 constructor(cx,cz){this.cx=cx;this.cz=cz;this.data=new Uint8Array(VOLUME);this.mesh=null;this.dirty=true}
 get(x,y,z){return y<0||y>=CHUNK_HEIGHT||x<0||x>=CHUNK_SIZE||z<0||z>=CHUNK_SIZE?0:this.data[idx(x,y,z)]}
 set(x,y,z,b){if(x<0||x>=CHUNK_SIZE||y<0||y>=CHUNK_HEIGHT||z<0||z>=CHUNK_SIZE)return;this.data[idx(x,y,z)]=b;this.dirty=true}
}

export class ChunkWorld{
 constructor(scene,seed=1337){this.scene=scene;this.seed=seed>>>0;this.chunks=new Map();this.renderRadius=4}
 key(cx,cz){return `${cx}|${cz}`}
 hash2(x,z){let h=(this.seed^Math.imul(x,0x45d9f3b)^Math.imul(z,0x119de1f3))>>>0;h=Math.imul(h^(h>>>16),0x27d4eb2d)>>>0;h=Math.imul(h^(h>>>15),0x85ebca6b)>>>0;return((h^(h>>>13))>>>0)/4294967295}
 height(wx,wz){const a=this.hash2(Math.floor(wx/32),Math.floor(wz/32)),b=this.hash2(Math.floor(wx/8),Math.floor(wz/8)),c=this.hash2(wx,wz);return Math.max(2,Math.min(CHUNK_HEIGHT-2,Math.floor(10+a*16+b*7+c*3)))}
 generateChunk(cx,cz){const c=new Chunk(cx,cz);for(let z=0;z<CHUNK_SIZE;z++)for(let x=0;x<CHUNK_SIZE;x++){const wx=cx*CHUNK_SIZE+x,wz=cz*CHUNK_SIZE+z,h=this.height(wx,wz);for(let y=0;y<=h;y++)c.set(x,y,z,y===h?(h<=7?blockIndex.sand:blockIndex.grass):y>=h-3?blockIndex.dirt:blockIndex.stone);const r=this.hash2(wx+91,wz-37);if(r>.975&&h>8&&x>2&&x<13&&z>2&&z<13){for(let y=1;y<=4&&h+y<CHUNK_HEIGHT;y++)c.set(x,h+y,z,blockIndex.wood);for(let dx=-2;dx<=2;dx++)for(let dz=-2;dz<=2;dz++)if(Math.abs(dx)+Math.abs(dz)<4)c.set(x+dx,h+4,z+dz,blockIndex.leaves)}}this.chunks.set(this.key(cx,cz),c);return c}
 ensureChunk(cx,cz){return this.chunks.get(this.key(cx,cz))||this.generateChunk(cx,cz)}
 peekBlock(wx,wy,wz){if(wy<0||wy>=CHUNK_HEIGHT)return 0;const c=this.chunks.get(this.key(Math.floor(wx/CHUNK_SIZE),Math.floor(wz/CHUNK_SIZE)));return c?c.get(wrap(wx),wy,wrap(wz)):0}
 getBlock(wx,wy,wz){if(wy<0||wy>=CHUNK_HEIGHT)return 0;const c=this.ensureChunk(Math.floor(wx/CHUNK_SIZE),Math.floor(wz/CHUNK_SIZE));return c.get(wrap(wx),wy,wrap(wz))}
 setBlock(wx,wy,wz,b){if(wy<0||wy>=CHUNK_HEIGHT)return false;const cx=Math.floor(wx/CHUNK_SIZE),cz=Math.floor(wz/CHUNK_SIZE),c=this.ensureChunk(cx,cz),lx=wrap(wx),lz=wrap(wz);c.set(lx,wy,lz,b);this.rebuildChunk(cx,cz);if(lx===0)this.rebuildChunk(cx-1,cz);if(lx===CHUNK_SIZE-1)this.rebuildChunk(cx+1,cz);if(lz===0)this.rebuildChunk(cx,cz-1);if(lz===CHUNK_SIZE-1)this.rebuildChunk(cx,cz+1);return true}
 rebuildChunk(cx,cz){const c=this.ensureChunk(cx,cz);if(c.mesh){this.scene.remove(c.mesh);c.mesh.geometry.dispose();c.mesh.material.dispose();c.mesh=null}const p=[],n=[],col=[],ind=[];for(let y=0;y<CHUNK_HEIGHT;y++)for(let z=0;z<CHUNK_SIZE;z++)for(let x=0;x<CHUNK_SIZE;x++){const b=c.get(x,y,z);if(!b)continue;const wx=cx*CHUNK_SIZE+x,wz=cz*CHUNK_SIZE+z,bc=new THREE.Color(BLOCKS[b].color);for(const f of faces){if(this.peekBlock(wx+f.n[0],y+f.n[1],wz+f.n[2])!==0)continue;const base=p.length/3;for(const v of f.v){p.push(wx+v[0],y+v[1],wz+v[2]);n.push(...f.n);col.push(bc.r,bc.g,bc.b)}ind.push(base,base+1,base+2,base,base+2,base+3)}}const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.Float32BufferAttribute(p,3));g.setAttribute('normal',new THREE.Float32BufferAttribute(n,3));g.setAttribute('color',new THREE.Float32BufferAttribute(col,3));g.setIndex(ind);const m=new THREE.MeshLambertMaterial({vertexColors:true});c.mesh=new THREE.Mesh(g,m);c.mesh.userData={cx,cz};this.scene.add(c.mesh);c.dirty=false}
 stream(px,pz){const pcx=Math.floor(px/CHUNK_SIZE),pcz=Math.floor(pz/CHUNK_SIZE),wanted=[];for(let dz=-this.renderRadius;dz<=this.renderRadius;dz++)for(let dx=-this.renderRadius;dx<=this.renderRadius;dx++)if(dx*dx+dz*dz<=this.renderRadius*this.renderRadius){wanted.push([pcx+dx,pcz+dz]);this.ensureChunk(pcx+dx,pcz+dz)}for(const [cx,cz] of wanted){const c=this.chunks.get(this.key(cx,cz));if(c?.dirty)this.rebuildChunk(cx,cz)}const keep=new Set(wanted.map(([x,z])=>this.key(x,z)));for(const [k,c] of this.chunks){if(!keep.has(k)){if(c.mesh){this.scene.remove(c.mesh);c.mesh.geometry.dispose();c.mesh.material.dispose()}this.chunks.delete(k)}}}
}
