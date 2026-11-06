from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
path=root/'Config/DefaultEngine.ini'
text=path.read_text(encoding='utf-8-sig')
updates={'EditorStartupMap':'/Game/LastBroadcast/Maps/L_LastBroadcast.L_LastBroadcast','GameDefaultMap':'/Game/LastBroadcast/Maps/L_LastBroadcast.L_LastBroadcast','GlobalDefaultGameMode':'/Game/LastBroadcast/Blueprints/BP_LBGameMode.BP_LBGameMode_C','r.ReflectionMethod':'0','r.DynamicGlobalIlluminationMethod':'0','r.GenerateMeshDistanceFields':'False','r.RayTracing':'False','r.Shadow.Virtual.Enable':'0','bUseSplitscreen':'False'}
for key,value in updates.items():text=re.sub(r'^'+re.escape(key)+r'=.*$',key+'='+value,text,flags=re.M)
if '[SystemSettings]' not in text:text+='\n[SystemSettings]\nr.AntiAliasingMethod=0\nr.ScreenPercentage=75\nr.MotionBlurQuality=0\nr.EyeAdaptationQuality=0\nr.Tonemapper.Sharpen=0.3\n'
path.write_text(text,encoding='utf-8')
path=root/'Config/DefaultInput.ini';text=path.read_text(encoding='utf-8-sig')
if 'AxisName="LBForward"' not in text:
    text=text.replace('[/Script/Engine.InputSettings]','[/Script/Engine.InputSettings]\n+AxisMappings=(AxisName="LBForward",Scale=1.0,Key=W)\n+AxisMappings=(AxisName="LBForward",Scale=-1.0,Key=S)\n+AxisMappings=(AxisName="LBRight",Scale=1.0,Key=D)\n+AxisMappings=(AxisName="LBRight",Scale=-1.0,Key=A)',1)
path.write_text(text,encoding='utf-8')
path=root/'Config/DefaultGame.ini';text=path.read_text(encoding='utf-8-sig') if path.exists() else ''
if 'LastBroadcast/Fonts' not in text:text+='\n[/Script/UnrealEd.ProjectPackagingSettings]\n+MapsToCook=(FilePath="/Game/LastBroadcast/Maps/L_LastBroadcast")\n+DirectoriesToAlwaysStageAsUFS=(Path="LastBroadcast/Fonts")\n'
path.write_text(text,encoding='utf-8')
print('LastBroadcast is the default map; retro rendering and packaging paths configured.')
