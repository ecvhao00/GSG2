"""Deterministic hand-built prototype textures and sound effects. No generated AI imagery."""
from pathlib import Path
from PIL import Image, ImageDraw
import math, random, wave, struct, shutil

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Content/LastBroadcast/SourceAssets'
OUT.mkdir(parents=True,exist_ok=True)
rng=random.Random(1023)
for name,base in [('Plaster',(148,157,143)),('Tile',(62,83,75)),('Wood',(82,54,41)),('Board',(22,48,40)),('Metal',(81,99,98))]:
    im=Image.new('RGB',(64,64)); pix=im.load()
    for y in range(64):
        for x in range(64):
            noise=rng.randrange(-13,14)
            if name=='Tile': noise+=12 if (x//32+y//32)%2 else -5; noise-=32 if x%32<2 or y%32<2 else 0
            if name=='Wood':noise+=int(10*math.sin(y*.8+x*.15))
            if name=='Plaster' and rng.random()<.035:noise-=30
            pix[x,y]=tuple(max(0,min(255,c+noise)) for c in base)
    if name=='Board':
        draw=ImageDraw.Draw(im)
        for i in range(7):
            y=10+i*6; draw.line((6,y,36+rng.randrange(15),y),fill=(91,110,90),width=1)
    im.save(OUT/f'{name}.png')

RATE=22050
def save(name,seconds,fn):
    with wave.open(str(OUT/f'{name}.wav'),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE)
        data=bytearray()
        for n in range(int(RATE*seconds)):
            t=n/RATE; value=max(-.94,min(.94,fn(t)))
            data.extend(struct.pack('<h',int(value*32767)))
        w.writeframes(data)

def chime(t):
    v=0
    for at,f in [(0,523.25),(.58,659.25),(1.16,587.33),(1.74,392)]:
        a=t-at
        if a>=0:v+=(math.sin(2*math.pi*f*a)+.25*math.sin(2*math.pi*f*2.01*a))*.29*math.exp(-a*3.4)
    return v*.75+rng.uniform(-.008,.008)
save('Chime',3,chime)
save('Footstep',.29,lambda t:(rng.uniform(-1,1)*.32+math.sin(2*math.pi*78*t)*.35)*math.exp(-t*24))
save('Door',.8,lambda t:(rng.uniform(-1,1)*.20*math.exp(-t*5)+math.sin(2*math.pi*(87+20*t)*t)*.22*math.exp(-t*4))+(rng.uniform(-.45,.45)*math.exp(-(t-.56)*42) if t>=.56 else 0))
save('Warning',1,lambda t:math.sin(2*math.pi*(190+250*t)*t)*.23*math.exp(-t*1.7)+rng.uniform(-.06,.06)*math.exp(-t*4))
save('Alarm',2.8,lambda t:math.sin(2*math.pi*(650+130*math.sin(2*math.pi*1.2*t))*t)*.14*(.8+.2*math.sin(2*math.pi*1.2*t)))
save('Hum',12,lambda t:math.sin(2*math.pi*60*t)*.025+math.sin(2*math.pi*120*t)*.01+rng.uniform(-.015,.015))
save('IntroVoice',5,lambda t:rng.uniform(-.035,.035)+math.sin(2*math.pi*60*t)*.012)
fonts=ROOT/'Content/LastBroadcast/Fonts';fonts.mkdir(exist_ok=True)
enginefonts=Path('C:/Program Files/Epic Games/UE_5.8/Engine/Content/Slate/Fonts')
shutil.copy2(enginefonts/'DroidSansFallback.ttf',fonts/'DroidSansFallback.ttf')
shutil.copy2(enginefonts/'DroidSans.tps',fonts/'DroidSans.tps')
print(f'Source assets prepared in {OUT}')
