// Reproducible raster resizing/compression. Originals never enter the app bundle.
import sharp from 'sharp';
import {mkdir,stat,writeFile} from 'node:fs/promises';
const source='design/brand/pocket-engineer-master.png';
await mkdir('www/assets',{recursive:true});
const outputs=[];
async function record(path){outputs.push({path,bytes:(await stat(path)).size});}
for(const size of [32,180,192,512]){
  const path=`www/assets/icon-${size}.png`;
  await sharp(source).resize(size,size).png({palette:true,colours:16,dither:0,compressionLevel:9}).toFile(path);await record(path);
}
await sharp(source).resize(192,192).webp({quality:85,effort:6}).toFile('www/assets/logo.webp');await record('www/assets/logo.webp');
for(const name of ['algebra','circuit','linear-algebra','logic']){
  const path=`www/assets/${name}.webp`;
  await sharp(`design/illustrations/${name}.png`).resize(480,480).webp({quality:76,effort:6}).toFile(path);await record(path);
}
for(const [density,size] of Object.entries({mdpi:48,hdpi:72,xhdpi:96,xxhdpi:144,xxxhdpi:192})){
  const dir=`android/app/src/main/res/mipmap-${density}`;await mkdir(dir,{recursive:true});
  await sharp(source).resize(size,size).png({palette:true,colours:16,dither:0,compressionLevel:9}).toFile(`${dir}/ic_launcher.png`);
}
await mkdir('android/app/src/main/res/drawable-nodpi',{recursive:true});
await sharp(source).resize(432,432).png({palette:true,colours:16,dither:0,compressionLevel:9}).toFile('android/app/src/main/res/drawable-nodpi/ic_launcher_foreground.png');
await writeFile('docs/generated/ASSET_SIZES.json',JSON.stringify({mode:'Built-in image generation, followed by sharp raster compression',outputs},null,2)+'\n');
console.log(outputs);
