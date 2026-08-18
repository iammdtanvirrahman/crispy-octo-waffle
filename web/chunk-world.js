import * as THREE from 'three';

export const CHUNK_SIZE = 16;
export const CHUNK_HEIGHT = 64;
const VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_HEIGHT;
export const BLOCKS = [
  {id:'air',color:0x000000},{id:'grass',color:0x63a843},{id:'dirt',color:0x8a5a36},{id:'stone',color:0x7e858d},
  {id:'wood',color:0x9b6b3c},{id:'sand',color:0xd9c47f},{id:'leaves',color:0x3f8c4d}
];
const blockIndex=Object.fromEntries(BLOCKS.map((b,i)=>[b.id,i]));
const idx=(x,y,z)=>y*CHUNK_SIZE*CHUNK_SIZE+z*CHUNK_SIZE+x;
const wrap=n=>((n%CHUNK_SIZE)+CHUNK_SIZE)%CHUNK_SIZE;
const solid=b=>b!==0;

export class Chunk{
  constructor(cx,cz){this.cx=cx;this.cz=cz;this.data=new Uint8Array(VOLUME);this.mesh=null;this.dirty=true}
  get(x,y,z){return y<0||y>=CHUNK_HEIGHT||x<0||x>=CHUNK_SIZE||z<0||z>=CHUNK_SIZE?0:this.data[idx(x,y,z)]}
  set(x,y,z,b){if(x<0||x>=CHUNK_SIZE||y<0||y>=CHUNK_HEIGHT||z<0||z>=CHUNK_SIZE)return;this.data[idx(x,y,z)]=b;this.dirty=true}
}

export class ChunkWorld{
  constructor(scene,seed=1337){this.scene=scene;this.seed=seed>>>0;this.chunks=new Map();this.renderRadius=4;this.maxRebuildsPerFrame=2;this.rebuildQueue=[];this.rebuildQueued=new Set()}
  key(cx,cz){return `${cx}|${cz}`}
  hash2(x,z){let h=(this.seed^Math.imul(x,0x45d9f3b)^Math.imul(z,0x119de1f3))>>>0;h=Math.imul(h^(h>>>16),0x27d4eb2d)>>>0;h=Math.imul(h^(h>>>15),0x85ebca6b)>>>0;return((h^(h>>>13))>>>0)/4294967295}
  height(wx,wz){const a=this.hash2(Math.floor(wx/32),Math.floor(wz/32)),b=this.hash2(Math.floor(wx/8),Math.floor(wz/8)),c=this.hash2(wx,wz);return Math.max(2,Math.min(CHUNK_HEIGHT-2,Math.floor(10+a*16+b*7+c*3)))}
  generateChunk(cx,cz){const c=new Chunk(cx,cz);for(let z=0;z<CHUNK_SIZE;z++)for(let x=0;x<CHUNK_SIZE;x++){const wx=cx*CHUNK_SIZE+x,wz=cz*CHUNK_SIZE+z,h=this.height(wx,wz);for(let y=0;y<=h;y++)c.set(x,y,z,y===h?(h<=7?blockIndex.sand:blockIndex.grass):y>=h-3?blockIndex.dirt:blockIndex.stone);const r=this.hash2(wx+91,wz-37);if(r>.975&&h>8&&x>2&&x<13&&z>2&&z<13){for(let y=1;y<=4&&h+y<CHUNK_HEIGHT;y++)c.set(x,h+y,z,blockIndex.wood);for(let dx=-2;dx<=2;dx++)for(let dz=-2;dz<=2;dz++)if(Math.abs(dx)+Math.abs(dz)<4)c.set(x+dx,h+4,z+dz,blockIndex.leaves)}}this.chunks.set(this.key(cx,cz),c);return c}
  ensureChunk(cx,cz){return this.chunks.get(this.key(cx,cz))||this.generateChunk(cx,cz)}
  peekBlock(wx,wy,wz){if(wy<0||wy>=CHUNK_HEIGHT)return 0;const c=this.chunks.get(this.key(Math.floor(wx/CHUNK_SIZE),Math.floor(wz/CHUNK_SIZE)));return c?c.get(wrap(wx),wy,wrap(wz)):0}
  getBlock(wx,wy,wz){if(wy<0||wy>=CHUNK_HEIGHT)return 0;const c=this.ensureChunk(Math.floor(wx/CHUNK_SIZE),Math.floor(wz/CHUNK_SIZE));return c.get(wrap(wx),wy,wrap(wz))}
  enqueueRebuild(cx,cz){const k=this.key(cx,cz);const c=this.chunks.get(k);if(!c||this.rebuildQueued.has(k))return;this.rebuildQueue.push([cx,cz]);this.rebuildQueued.add(k)}
  setBlock(wx,wy,wz,b){
    if(wy<0||wy>=CHUNK_HEIGHT)return false;
    const cx=Math.floor(wx/CHUNK_SIZE),cz=Math.floor(wz/CHUNK_SIZE),c=this.ensureChunk(cx,cz),lx=wrap(wx),lz=wrap(wz);
    c.set(lx,wy,lz,b);this.enqueueRebuild(cx,cz);
    if(lx===0)this.enqueueRebuild(cx-1,cz); if(lx===CHUNK_SIZE-1)this.enqueueRebuild(cx+1,cz);
    if(lz===0)this.enqueueRebuild(cx,cz-1); if(lz===CHUNK_SIZE-1)this.enqueueRebuild(cx,cz+1); return true;
  }

  addQuad(p,n,col,ind,a,b,c,d,origin,du,dv){
    const base=p.length/3; const pts=[origin,[origin[0]+du[0]*a,origin[1]+du[1]*a,origin[2]+du[2]*a],[origin[0]+du[0]*a+dv[0]*b,origin[1]+du[1]*a+dv[1]*b,origin[2]+du[2]*a+dv[2]*b],[origin[0]+dv[0]*b,origin[1]+dv[1]*b,origin[2]+dv[2]*b]];
    for(const q of pts){p.push(q[0],q[1],q[2]);n.push(n[0]??0,n[1]??0,n[2]??0)}
    ind.push(base,base+1,base+2,base,base+2,base+3);
  }

  rebuildChunk(cx,cz){
    const c=this.ensureChunk(cx,cz);
    if(c.mesh){this.scene.remove(c.mesh);c.mesh.geometry.dispose();c.mesh.material.dispose();c.mesh=null}
    const p=[],normals=[],colors=[],ind=[];
    const emit=(axis,positive,u,v,w,plane,iu,iv,du,dv,block)=>{
      const nx=axis===0?(positive?1:-1):0,ny=axis===1?(positive?1:-1):0,nz=axis===2?(positive?1:-1):0;
      const base=p.length/3;
      const verts=[
        [w[0]+du[0]*iu+dv[0]*iv,w[1]+du[1]*iu+dv[1]*iv,w[2]+du[2]*iu+dv[2]*iv],
        [w[0]+du[0]*(iu+u)+dv[0]*iv,w[1]+du[1]*(iu+u)+dv[1]*iv,w[2]+du[2]*(iu+u)+dv[2]*iv],
        [w[0]+du[0]*(iu+u)+dv[0]*(iv+v),w[1]+du[1]*(iu+u)+dv[1]*(iv+v),w[2]+du[2]*(iu+u)+dv[2]*(iv+v)],
        [w[0]+du[0]*iu+dv[0]*(iv+v),w[1]+du[1]*iu+dv[1]*(iv+v),w[2]+du[2]*iu+dv[2]*(iv+v)]
      ];
      const cc=new THREE.Color(BLOCKS[block].color);
      for(const q of verts){p.push(...q);normals.push(nx,ny,nz);colors.push(cc.r,cc.g,cc.b)}
      if(positive) ind.push(base,base+1,base+2,base,base+2,base+3); else ind.push(base,base+2,base+1,base,base+3,base+2);
    };

    // Greedy rectangles for each axis. Mask entries are block ids, or 0 for no visible face.
    const dims=[CHUNK_SIZE,CHUNK_HEIGHT,CHUNK_SIZE];
    for(let axis=0;axis<3;axis++){
      const a1=(axis+1)%3,a2=(axis+2)%3;
      const uSize=dims[a1],vSize=dims[a2],wSize=dims[axis];
      const mask=new Int16Array(uSize*vSize);
      for(let side=0;side<2;side++){
        for(let slice=0;slice<=wSize;slice++){
          for(let j=0;j<vSize;j++) for(let i=0;i<uSize;i++){
            const pos=[0,0,0],neg=[0,0,0];
            pos[axis]=slice; pos[a1]=i; pos[a2]=j;
            neg[axis]=slice-1; neg[a1]=i; neg[a2]=j;
            const A=this.getBlock(cx*CHUNK_SIZE+pos[0],pos[1],cz*CHUNK_SIZE+pos[2]);
            const B=this.getBlock(cx*CHUNK_SIZE+neg[0],neg[1],cz*CHUNK_SIZE+neg[2]);
            mask[j*uSize+i]=side===0?(solid(A)&&!solid(B)?A:0):(!solid(A)&&solid(B)?B:0);
          }
          for(let j=0;j<vSize;j++){
            for(let i=0;i<uSize;){
              const block=mask[j*uSize+i]; if(!block){i++;continue}
              let width=1; while(i+width<uSize&&mask[j*uSize+i+width]===block)width++;
              let height=1; outer: while(j+height<vSize){for(let k=0;k<width;k++)if(mask[(j+height)*uSize+i+k]!==block)break outer;height++}
              for(let y=0;y<height;y++)for(let x=0;x<width;x++)mask[(j+y)*uSize+i+x]=0;
              const origin=[0,0,0];origin[axis]=slice;origin[a1]=i;origin[a2]=j;
              const du=[0,0,0],dv=[0,0,0];du[a1]=width;dv[a2]=height;
              const normalPositive=side===0;emit(axis,normalPositive,width,height,0,slice,i,j,du,dv,block);
              i+=width;
            }
          }
        }
      }
    }
    // Above emits local chunk coordinates; convert them to world coordinates after geometry creation.
    for(let i=0;i<p.length;i+=3){p[i]+=cx*CHUNK_SIZE;p[i+2]+=cz*CHUNK_SIZE}
    const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.Float32BufferAttribute(p,3));g.setAttribute('normal',new THREE.Float32BufferAttribute(normals,3));g.setAttribute('color',new THREE.Float32BufferAttribute(colors,3));g.setIndex(ind);g.computeBoundingSphere();
    const m=new THREE.MeshLambertMaterial({vertexColors:true});c.mesh=new THREE.Mesh(g,m);c.mesh.userData={cx,cz};this.scene.add(c.mesh);c.dirty=false;
  }

  processRebuildQueue(){let n=0;while(this.rebuildQueue.length&&n<this.maxRebuildsPerFrame){const [cx,cz]=this.rebuildQueue.shift();this.rebuildQueued.delete(this.key(cx,cz));if(this.chunks.has(this.key(cx,cz)))this.rebuildChunk(cx,cz);n++}}
  stream(px,pz){
    const pcx=Math.floor(px/CHUNK_SIZE),pcz=Math.floor(pz/CHUNK_SIZE),wanted=[];
    for(let dz=-this.renderRadius;dz<=this.renderRadius;dz++)for(let dx=-this.renderRadius;dx<=this.renderRadius;dx++)if(dx*dx+dz*dz<=this.renderRadius*this.renderRadius){wanted.push([pcx+dx,pcz+dz]);const c=this.ensureChunk(pcx+dx,pcz+dz);if(c.dirty)this.enqueueRebuild(c.cx,c.cz)}
    const keep=new Set(wanted.map(([x,z])=>this.key(x,z)));for(const [k,c] of this.chunks){if(!keep.has(k)){if(c.mesh){this.scene.remove(c.mesh);c.mesh.geometry.dispose();c.mesh.material.dispose()}this.chunks.delete(k);this.rebuildQueued.delete(k)}}
    this.processRebuildQueue();
  }
}
