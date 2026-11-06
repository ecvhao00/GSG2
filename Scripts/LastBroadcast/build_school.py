"""Run with UnrealEditor-Cmd -run=pythonscript -script=... . Rebuilds only /Game/LastBroadcast."""
import unreal as u
from pathlib import Path
import json, math, wave, shutil, datetime

ROOT=Path(u.Paths.project_dir())
BASE='/Game/LastBroadcast'
MAP=BASE+'/Maps/L_LastBroadcast'
tools=u.AssetToolsHelpers.get_asset_tools()
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
u.EditorAssetLibrary.make_directory(BASE+'/Maps')
u.EditorAssetLibrary.make_directory(BASE+'/Materials')
u.EditorAssetLibrary.make_directory(BASE+'/Audio')
u.EditorAssetLibrary.make_directory(BASE+'/Textures')
u.EditorAssetLibrary.make_directory(BASE+'/Blueprints')

def load(path):return u.load_asset(path)
def asset(name,folder,cls,factory):
    package='/'.join(x for x in [BASE,folder] if x)
    old=load(package+'/'+name)
    return old or tools.create_asset(name,package,cls,factory)

tasks=[]
for ext,folder in [('png','Textures'),('wav','Audio')]:
    for source in (ROOT/'Content/LastBroadcast/SourceAssets').glob('*.'+ext):
        t=u.AssetImportTask();t.filename=str(source);t.destination_path=BASE+'/'+folder;t.destination_name=source.stem;t.automated=True;t.replace_existing=True;t.save=True;tasks.append(t)
tools.import_asset_tasks(tasks)
for t in tasks:
    if t.filename.endswith('.png'):
        tex=load(BASE+'/Textures/'+Path(t.filename).stem)
        tex.set_editor_property('filter',u.TextureFilter.TF_NEAREST)
        tex.set_editor_property('lod_group',u.TextureGroup.TEXTUREGROUP_WORLD)

atten=asset('A_School','Audio',u.SoundAttenuation,u.SoundAttenuationFactory())
att=u.SoundAttenuationSettings()
att.set_editor_property('attenuate',True);att.set_editor_property('spatialize',True)
att.set_editor_property('attenuation_shape_extents',u.Vector(150,0,0));att.set_editor_property('falloff_distance',5200)
atten.set_editor_property('attenuation',att)
for name in ['Chime','Footstep','Door','Alarm','Warning']:
    snd=load(BASE+'/Audio/'+name);snd.set_editor_property('attenuation_settings',atten)
hum=load(BASE+'/Audio/Hum');hum.set_editor_property('looping',True)

def material(name,emissive=False):
    m=load(BASE+'/Materials/'+name)
    if m:return m
    m=tools.create_asset(name,BASE+'/Materials',u.Material,u.MaterialFactoryNew())
    edit=u.MaterialEditingLibrary
    color=edit.create_material_expression(m,u.MaterialExpressionVectorParameter,-500,0);color.set_editor_property('parameter_name','Color');color.set_editor_property('default_value',u.LinearColor(.5,.6,.55,1))
    if emissive:
        m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
        edit.connect_material_property(color,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        sample=edit.create_material_expression(m,u.MaterialExpressionTextureSampleParameter2D,-500,200);sample.set_editor_property('parameter_name','Albedo');sample.set_editor_property('texture',load(BASE+'/Textures/Plaster'))
        mul=edit.create_material_expression(m,u.MaterialExpressionMultiply,-200,0)
        edit.connect_material_expressions(color,'',mul,'A');edit.connect_material_expressions(sample,'RGB',mul,'B');edit.connect_material_property(mul,'',u.MaterialProperty.MP_BASE_COLOR)
        rough=edit.create_material_expression(m,u.MaterialExpressionConstant,-200,300);rough.set_editor_property('r',.95);edit.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
    edit.recompile_material(m);return m

lit=material('M_SchoolSurface');emission=material('M_SchoolEmission',True)
palette={}
def mi(name,color,texture='Plaster',unlit=False):
    factory=u.MaterialInstanceConstantFactoryNew()
    m=asset('MI_'+name,'Materials',u.MaterialInstanceConstant,factory)
    u.MaterialEditingLibrary.set_material_instance_parent(m,emission if unlit else lit)
    u.MaterialEditingLibrary.set_material_instance_vector_parameter_value(m,'Color',u.LinearColor(*color,1))
    if not unlit:u.MaterialEditingLibrary.set_material_instance_texture_parameter_value(m,'Albedo',load(BASE+'/Textures/'+texture))
    palette[name]=m;return m
mi('Wall',(.85,.91,.83));mi('LowerWall',(.40,.58,.48));mi('Floor',(.68,.83,.72),'Tile');mi('Ceiling',(.7,.76,.69))
mi('Wood',(.9,.72,.56),'Wood');mi('DarkWood',(.30,.34,.30),'Wood');mi('Metal',(.50,.70,.68),'Metal')
mi('Board',(.7,.8,.65),'Board');mi('Paper',(.93,.9,.71));mi('Black',(.035,.045,.05))
mi('GreenGlow',(.25,.9,.62),unlit=True);mi('BlueGlow',(.11,.2,.40),unlit=True);mi('RedGlow',(.7,.035,.015),unlit=True);mi('Lamp',(.65,.74,.57),unlit=True)

def blueprint(name,parent):
    fac=u.BlueprintFactory();fac.set_editor_property('parent_class',parent)
    return asset(name,'Blueprints',u.Blueprint,fac)
bpplayer=blueprint('BP_LBPlayer',u.LBPlayer)
bpgame=blueprint('BP_LBGameMode',u.LBGameMode)
bpdoor=blueprint('BP_LBDoor',u.LBDoor)
bpdevice=blueprint('BP_LBDevice',u.LBDevice)
bpenemy=blueprint('BP_LBEnemy',u.LBEnemy)
bpdirector=blueprint('BP_LBDirector',u.LBDirector)
u.get_default_object(bpgame.generated_class()).set_editor_property('default_pawn_class',bpplayer.generated_class())

fac=u.DataAssetFactory();fac.set_editor_property('data_asset_class',u.LBSettings)
tuning=asset('DA_PrototypeTuning','',u.LBSettings,fac)
for prop,name in [('chime','Chime'),('intro_voice','IntroVoice'),('footstep','Footstep'),('door_sound','Door'),('alarm','Alarm'),('warning','Warning'),('hum','Hum')]:
    tuning.set_editor_property(prop,load(BASE+'/Audio/'+name))
with wave.open(str(ROOT/'Content/LastBroadcast/SourceAssets/IntroVoice.wav')) as voice:
    tuning.set_editor_property('intro_time',max(10.,math.ceil(voice.getnframes()/voice.getframerate())+4.))
tuning.set_editor_property('required_puzzle_ids',['MID_A','MID_B'])
tuning.set_editor_property('puzzle_content_ready',False)
tuning.set_editor_property('enable_prototype_puzzles',True)

if u.EditorAssetLibrary.does_asset_exist(MAP):
    revision=ROOT/'Backup/LastBroadcast_AutoRevisions'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    revision.mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'Content/LastBroadcast/Maps/L_LastBroadcast.umap',revision/'L_LastBroadcast.umap')
    if not levels.load_level(MAP):raise RuntimeError('Could not load generated LastBroadcast level')
    for old_actor in actors.get_all_level_actors():
        if str(old_actor.get_folder_path()).startswith('LastBroadcast/'):
            actors.destroy_actor(old_actor)
else:
    if not levels.new_level(MAP):raise RuntimeError('Could not create LastBroadcast level')
cube=load('/Engine/BasicShapes/Cube');sphere=load('/Engine/BasicShapes/Sphere')
count=0
def label(actor,name,folder='Architecture'):
    actor.set_actor_label(name);actor.set_folder_path('LastBroadcast/'+folder);return actor
def box(name,pos,scale,mat='Wall',collision=True,folder='Architecture'):
    global count
    a=label(actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*pos)),name,folder)
    c=a.static_mesh_component;c.set_static_mesh(cube);c.set_material(0,palette[mat]);c.set_editor_property('mobility',u.ComponentMobility.STATIC)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision');a.set_actor_scale3d(u.Vector(*(v/100 for v in scale)));count+=1;return a
def text(name,value,pos,yaw=90,size=23,color=(.75,.85,.74),folder='Signs'):
    a=label(actors.spawn_actor_from_class(u.TextRenderActor,u.Vector(*pos),u.Rotator(pitch=0,yaw=yaw+180,roll=0)),name,folder)
    c=a.text_render;c.set_text(value);c.set_world_size(size);c.set_text_render_color(u.Color(*(int(v*255) for v in color),255));c.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER);return a
def light(name,pos,intensity=1700,color=(.62,.78,.65),radius=950,powered=False):
    a=label(actors.spawn_actor_from_class(u.PointLight,u.Vector(*pos)),name,'Lighting')
    c=a.point_light_component;c.set_intensity(intensity*.02);c.set_attenuation_radius(radius);c.set_light_color(u.LinearColor(*color,1));c.set_cast_shadows(powered or name.startswith('Courtyard'))
    if powered:a.tags=['LB_Power'];c.set_visibility(False)
    return a
def wall_y(name,y,x1,x2,openings=()):
    edges=[x1]
    for cx,w in sorted(openings):edges += [cx-w/2,cx+w/2]
    edges += [x2]
    for i in range(0,len(edges),2):
        a,b=edges[i:i+2]
        if b>a:
            box(name+'_wall',((a+b)/2,y,175),(b-a,20,350))
            box(name+'_wainscot',((a+b)/2,y-11,65),(b-a,3,130),'LowerWall',False)
            box(name+'_baseboard',((a+b)/2,y-14,8),(b-a,6,16),'DarkWood',False)
    for cx,w in openings:box(name+'_lintel',(cx,y,300),(w,20,100))
def wall_x(name,x,y1,y2,openings=()):
    edges=[y1]
    for cy,w in sorted(openings):edges += [cy-w/2,cy+w/2]
    edges += [y2]
    for i in range(0,len(edges),2):
        a,b=edges[i:i+2]
        if b>a:box(name+'_wall',(x,(a+b)/2,175),(20,b-a,350));box(name+'_wainscot',(x-11,(a+b)/2,65),(3,b-a,130),'LowerWall',False)
    for cy,w in openings:box(name+'_lintel',(x,cy,300),(20,w,100))
def door(id,x,y,horizontal=True,external=False,opened=True):
    p=u.Vector(x+65,y,0) if horizontal else u.Vector(x,y-65,0)
    a=label(actors.spawn_actor_from_class(bpdoor.generated_class(),p,u.Rotator(pitch=0,yaw=90 if horizontal else 0,roll=0)),id,'Doors')
    a.set_editor_property('door_id',id);a.set_editor_property('external',external);a.set_editor_property('default_open',opened);a.leaf.set_material(0,palette['DarkWood'])
    return a
def desk(x,y,teacher=False):
    w=125 if teacher else 68;d=65 if teacher else 48
    box('Desk',(x,y,76),(w,d,8),'Wood',True,'Furniture')
    for dx in [-w*.38,w*.38]:
        for dy in [-d*.35,d*.35]:box('Desk_leg',(x+dx,y+dy,36),(4,4,72),'Metal',False,'Furniture')
    box('Chair_seat',(x,y+63,44),(40,40,6),'Wood',True,'Furniture');box('Chair_back',(x,y+81,69),(40,5,50),'Wood',True,'Furniture')
def device(id,kind,title,pos,scale=(38,18,55),mat='Metal',yaw=90,requires=True,note='',point=None):
    a=label(actors.spawn_actor_from_class(bpdevice.generated_class(),u.Vector(*pos),u.Rotator(pitch=0,yaw=yaw,roll=0)),id,'Devices')
    a.set_editor_property('kind',kind);a.set_editor_property('stable_id',id);a.set_editor_property('display_name',title);a.set_editor_property('requires_power',requires);a.set_editor_property('note_text',note)
    a.mesh.set_static_mesh(cube);a.mesh.set_material(0,palette[mat]);a.set_actor_scale3d(u.Vector(*(v/100 for v in scale)))
    if point is not None:a.set_editor_property('investigation_point',u.Vector(*point))
    return a

# Main building 40 x 28m, two loops and a central north/south passage.
box('School_floor',(0,0,-18),(4020,2820,36),'Floor')
box('Broadcast_floor',(2400,550,-18),(800,1700,36),'Floor')
box('Entrance_floor',(1650,1700,-18),(700,600,36),'Floor')
box('Courtyard',(400,2400,-18),(5600,1100,36),'Floor')
box('Roof',(0,0,365),(4040,2840,30),'DarkWood')
box('Broadcast_roof',(2400,550,365),(820,1720,30),'DarkWood')
box('Entrance_roof',(1650,1700,365),(720,620,30),'DarkWood')
wall_y('North',-1400,-2000,2000);wall_x('West',-2000,-1400,1400)
wall_y('South',1400,-2000,2000,[(1700,150)])
wall_x('East',2000,-1400,1400,[(0,150),(1100,150)])
wall_y('North_rooms',-600,-2000,2000,[(-1450,140),(-600,140),(300,140),(800,140),(1640,140)])
wall_x('Power_partition',-800,-1400,-600);wall_x('Class_partition',600,-1400,-600)
wall_x('Archive_west',-1650,-240,1040);wall_x('Archive_east',-120,-240,1040)
wall_y('Archive_north',-240,-1650,-120,[(-950,140)]);wall_y('Archive_south',1040,-1650,-120,[(-950,140)])
wall_x('Office_west',240,-240,1040);wall_x('Office_east',1650,-240,1040)
wall_y('Office_north',-240,240,1650,[(800,140)]);wall_y('Office_south',1040,240,1650,[(1220,140)])
wall_y('Annex_north',-300,2000,2800);wall_x('Annex_east',2800,-300,1400);wall_y('Annex_south',1400,2000,2800)
wall_y('Prep_partition',450,2000,2800,[(2400,140)])
wall_x('Entrance_west',1300,1400,2000);wall_x('Entrance_east',2000,1400,2000)
wall_y('Entrance_outer',2000,1300,2000,[(1700,150)])
for id,x,y,h,e,o in [('D_POWER',-1450,-600,1,0,1),('D_C1_A',-600,-600,1,0,1),('D_C1_B',300,-600,1,0,0),('D_C2_A',800,-600,1,0,1),('D_C2_B',1640,-600,1,0,1),('D_ARCH_N',-950,-240,1,0,1),('D_ARCH_S',-950,1040,1,0,1),('D_OFF_N',800,-240,1,0,1),('D_OFF_S',1220,1040,1,0,1),('D_PREP',2000,0,0,0,1),('D_RADIO',2000,1100,0,0,1),('D_PREP_RADIO',2400,450,1,0,1),('D_ENTRANCE',1700,1400,1,0,1),('D_EXIT',1700,2000,1,1,1)]:door(id,x,y,bool(h),bool(e),bool(o))

# School furniture. The clear center aisles and separated doors remain traversable.
for room,cx in [('Class1',-100),('Class2',1250)]:
    for x in [cx-330,cx,cx+330]:
        for y in [-1080,-830]:desk(x,y)
    box(room+'_blackboard',(cx,-1382,185),(750,14,145),'Board')
    desk(cx,-1260,True)
    text(room+'_sign',room.upper(),(cx,-583,265),90,25)
    light(room+'_powered',(cx,-1020,285),3800,(.65,.8,.67),1000,True)
    box(room+'_lamp',(cx,-1000,345),(200,32,5),'Lamp',False)
for x in [-1440,-520]:
    for y in [100,600]:
        box('Archive_shelf',(x,y,110),(110,320,220),'DarkWood',True,'Furniture')
        for z in [45,100,155,210]:box('Shelf_books',(x-2,y,z),(112,280,22),'Paper',False,'Furniture')
for x,y in [(520,60),(1110,60),(520,650),(1110,650)]:desk(x,y,True)
box('Power_cabinets',(-1800,-1150,120),(150,170,240),'Metal',True,'Furniture')
box('Broadcast_table',(2480,850,75),(500,90,150),'DarkWood',True,'Furniture')
box('Radio_equipment',(2600,850,160),(180,65,32),'Metal',True,'Furniture')
for x in [2550,2600,2650]:box('Meters',(x,812,168),(25,3,11),'GreenGlow',False,'Furniture')
box('Prep_cabinet',(2670,-40,110),(120,260,220),'Metal',True,'Furniture')

for name,pos in [('POWER ROOM',(-1450,-580,265)),('ARCHIVE',(-950,-220,265)),('TEACHERS OFFICE',(800,-220,265)),('BROADCAST ROOM',(1982,1100,268))]:text(name,name,pos,90 if name!='BROADCAST ROOM' else 180,22)
text('Outside_school','OLD SCHOOL  /  ANNEX',(450,1420,280),90,42,(.46,.73,.62))
text('Exit_sign','EXIT',(1700,1425,285),-90,28,(.2,1,.55))
text('Exit_outer','EXIT',(1700,1978,280),90,28,(.2,1,.55))
text('Prep_sign','BROADCAST PREP',(2390,-280,260),90,22)
for x in [-1700,-1000,-300,500,1250,1830]:
    for y in [-420,1220]:
        light('Emergency_blue',(x,y,265),550,(.17,.35,.65),700)
        light('Corridor_power',(x,y,290),1400,(.55,.75,.57),800,True)
        box('Fluorescent',(x,y,347),(140,24,5),'Lamp',False)
for name,pos,color in [('Archive',(-1000,350,280),(.45,.65,.55)),('Office',(900,350,280),(.6,.68,.55)),('Power',(-1450,-1000,280),(.35,.52,.65)),('Broadcast',(2450,1020,285),(.55,.72,.60)),('Prep',(2400,100,285),(.35,.45,.65))]:light(name+'_light',pos,2600,color,1100,True)
light('Entrance_red',(1700,1800,270),2600,(1,.035,.015),700)
light('Broadcast_emergency',(2400,1200,270),800,(.14,.4,.24),800)
light('Courtyard_red',(500,2300,400),18000,(.8,.035,.015),2200)
light('Courtyard_blue',(1800,2200,450),12000,(.12,.2,.7),1500)

note=device('POWER_CLUE',u.LBDeviceKind.NOTE,'전력 복구 단서',(-100,-1370,145),(70,5,44),'Paper',requires=False,note='배전실 설비 점검 기록\n\n현관 비상 개방 장치는 교내 방송 전원과 연결되어 있다.\n먼저 배전실의 전력을 복구해야 한다.\n\n퍼즐의 실제 단서와 풀이 방식은 후속 기획에서 확정한다.')
text('Clue_label','MAINTENANCE NOTE',(-100,-1360,191),90,13)
device('P01',u.LBDeviceKind.POWER_PUZZLE,'배전실 전력 복구',(-1890,-850,145),(28,85,110),'Metal',yaw=0,requires=False)
text('Power_label','POWER / P01',(-1870,-850,220),0,18)
device('RADIO_GUIDE',u.LBDeviceKind.NOTE,'방송 장치 사용 안내',(900,-210,160),(70,5,45),'Paper',requires=False,note='방송 장치 사용 안내\n\n1. 방송실에서 교실 1, 교실 2, 자료실 중 한 곳을 선택한다.\n2. 방송 시작 버튼을 누르면 해당 방의 스피커가 울린다.\n3. 모든 구역은 20초의 재사용 시간을 공유한다.\n\n순찰자는 소리가 난 곳을 조사한다.\n하지만 당신을 보고 있다면 방송만으로 추격을 멈추지 않는다.\n방송할 곳과 다음에 이동할 곳을 함께 생각하자.')
device('MID_A',u.LBDeviceKind.MIDDLE_PUZZLE,'자료실 작업 [퍼즐 미정]',(-1580,360,135),(45,75,85),'Metal',yaw=0,point=(-1250,400,0))
device('MID_B',u.LBDeviceKind.MIDDLE_PUZZLE,'교실 1 작업 [퍼즐 미정]',(530,-1210,140),(32,75,85),'Metal',yaw=180,point=(300,-1190,0))
for id,title,pos,point in [('Z01','교실 1',(-100,-1370,280),(70,-1210,0)),('Z02','교실 2',(1250,-1370,280),(1420,-1210,0)),('Z03','자료실',(-1450,1000,280),(-1450,900,0))]:
    d=device(id,u.LBDeviceKind.SPEAKER,title,pos,(50,22,35),'Black',requires=True,point=point);d.mesh.set_collision_profile_name('NoCollision')
for id,title,x in [('Z01','교실 1',2220),('Z02','교실 2',2350),('Z03','자료실',2480)]:
    device(id,u.LBDeviceKind.ZONE_SELECT,title,(x,795,147),(72,22,42),'Metal');text(id+'_button',id,(x,777,183),90,15)
device('RADIO_START',u.LBDeviceKind.RADIO_START,'방송 시작',(2630,795,147),(78,24,42),'GreenGlow')
text('Radio_start_label','ON AIR',(2630,775,184),90,13)
device('EXIT_CONSOLE',u.LBDeviceKind.EXIT_CONSOLE,'현관 비상 개방 콘솔',(2770,1140,145),(22,110,85),'Metal',yaw=180)
text('Exit_console_label','EMERGENCY EXIT',(2750,1140,205),180,16)
for id,kind,pos,ext in [('ENTRY',u.LBDeviceKind.ENTRANCE_TRIGGER,(1700,1760,96),(270,100,160)),('ESCAPE',u.LBDeviceKind.ESCAPE_TRIGGER,(1700,2140,96),(180,90,160))]:
    d=device(id,kind,id,pos,(10,10,10),'Black',requires=False);d.mesh.set_visibility(False);d.mesh.set_collision_profile_name('NoCollision');d.set_editor_property('trigger_extent',u.Vector(*ext))

enemy=label(actors.spawn_actor_from_class(bpenemy.generated_class(),u.Vector(-300,-420,0)),'The_Patroller','Gameplay')
enemy.set_editor_property('patrol_points',[u.Vector(*p) for p in [(-300,-420,0),(-1820,-420,0),(-1820,1220,0),(40,1220,0),(40,-420,0),(1830,-420,0),(1830,1220,0),(40,1220,0)]])
for c in enemy.get_components_by_class(u.StaticMeshComponent):c.set_material(0,palette['Black'])
director=label(actors.spawn_actor_from_class(bpdirector.generated_class(),u.Vector(0,0,0)),'LastBroadcast_Director','Gameplay');director.set_editor_property('settings',tuning)
start=label(actors.spawn_actor_from_class(u.PlayerStart,u.Vector(1700,2180,96),u.Rotator(pitch=0,yaw=-90,roll=0)),'Start_Outside','Gameplay')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();world.get_world_settings().set_editor_property('default_game_mode',bpgame.generated_class())
pp=label(actors.spawn_actor_from_class(u.PostProcessVolume,u.Vector(0,0,0)),'Analog_Grade','Lighting');pp.set_editor_property('unbound',True)
post=pp.get_editor_property('settings')
for key,val in [('override_auto_exposure_min_brightness',True),('auto_exposure_min_brightness',1.),('override_auto_exposure_max_brightness',True),('auto_exposure_max_brightness',1.),('override_vignette_intensity',True),('vignette_intensity',.42),('override_film_grain_intensity',True),('film_grain_intensity',.28),('override_motion_blur_amount',True),('motion_blur_amount',0.),('override_bloom_intensity',True),('bloom_intensity',.2)]:post.set_editor_property(key,val)
pp.set_editor_property('settings',post)

# A dark crimson low-poly sky shell. No image assets or external asset downloads.
sky=label(actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(0,0,0)),'Crimson_sky','Atmosphere')
sky.static_mesh_component.set_static_mesh(sphere);sky.static_mesh_component.set_material(0,palette['RedGlow']);sky.static_mesh_component.set_collision_profile_name('NoCollision');sky.set_actor_scale3d(u.Vector(-200,200,200))
# Save editable assets and level; no changes to the original sample maps.
levels.save_current_level();u.EditorAssetLibrary.save_directory(BASE,only_if_is_dirty=False,recursive=True)
report={'map':MAP,'static_mesh_actors':count,'total_actors':len(actors.get_all_level_actors()),'rooms':8,'doors':14,'zones':['Z01','Z02','Z03'],'prototype_puzzles':['P01','MID_A','MID_B'],'intro_seconds':tuning.get_editor_property('intro_time')}
(ROOT/'Docs/LastBroadcast/scene_build.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('LB_SCENE_COMPLETE '+json.dumps(report))


