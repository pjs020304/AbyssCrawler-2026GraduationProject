# pyrefly: ignore [missing-import]
import unreal
import math
import random

def set_prop_safe(obj, prop_name, value):
    try:
        obj.set_editor_property(prop_name, value)
        return True
    except Exception:
        return False

def generate_deep_sea_ruins():
    editor_actor_subsys = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    
    # 임시 구조물 메쉬 로드 (기본 도형 사용, 나중에 심해용 에셋으로 교체 가능)
    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    cyl_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cylinder")
    
    if not cube_mesh or not cyl_mesh:
        unreal.log_error("엔진 기본 셰이프를 불러올 수 없습니다.")
        return
        
    # 플레이어 시작 지점이나 맵 중앙을 기준으로 설정
    center_location = unreal.Vector(0, 0, 1000)
    radius = 3500.0 # 반경 35m 내외로 흩뿌림
    num_structures = 25 # 스폰할 구조물 갯수
    
    spawned_actors = []
    
    unreal.log("심해 유적(구조물) 및 생물발광 라이트 스폰 시작...")
    
    for i in range(num_structures):
        # 원형 분포 + 랜덤 노이즈(흩뿌림 효과)
        angle = (i / num_structures) * 2 * math.pi
        r = radius * math.sqrt(random.random()) # 원 내부에 고르게 분포
        x = center_location.x + r * math.cos(angle)
        y = center_location.y + r * math.sin(angle)
        z = center_location.z + random.uniform(0, 500) # 공중에 띄워서 스폰
        
        location = unreal.Vector(x, y, z)
        # 잔해 느낌을 주기 위해 무작위 기울임 적용
        rotation = unreal.Rotator(random.uniform(-45, 45), random.uniform(0, 360), random.uniform(-45, 45))
        
        # 1. 구조물 액터 스폰
        actor = editor_actor_subsys.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
        actor.set_actor_label(f"DeepSeaRuin_{i}")
        
        sm_comp = actor.get_component_by_class(unreal.StaticMeshComponent)
        if sm_comp:
            sm_comp.set_static_mesh(random.choice([cube_mesh, cyl_mesh]))
            
        # 거대한 랜드마크 느낌을 위한 랜덤 스케일
        scale = random.uniform(5.0, 15.0)
        actor.set_actor_scale3d(unreal.Vector(scale, scale, scale * random.uniform(0.5, 3.0)))
        
        # 2. 생물발광(Bioluminescence) 라이트 스폰 - 약 40% 확률로 구조물에 부착
        if random.random() > 0.6:
            light_loc = location + unreal.Vector(0, 0, scale * 50.0)
            light_actor = editor_actor_subsys.spawn_actor_from_class(unreal.PointLight, light_loc, unreal.Rotator())
            light_actor.set_actor_label(f"RuinGlow_Light_{i}")
            
            # 라이트를 구조물에 부착
            light_actor.attach_to_actor(actor, unreal.Name(), unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
            
            pl_comp = light_actor.get_component_by_class(unreal.PointLightComponent)
            if pl_comp:
                set_prop_safe(pl_comp, "intensity", random.uniform(50.0, 250.0))
                # 심해에 어울리는 청록색(Cyan) 또는 돌연변이 오렌지색 부여
                color = unreal.Color(0, 200, 255, 255) if random.random() > 0.5 else unreal.Color(255, 100, 0, 255)
                set_prop_safe(pl_comp, "light_color", color)
                set_prop_safe(pl_comp, "attenuation_radius", random.uniform(1000.0, 2500.0))
                set_prop_safe(pl_comp, "cast_shadows", False) # 어두운 심해 렌더링 성능 최적화
                
            spawned_actors.append(light_actor)
            
        spawned_actors.append(actor)
        
    # 3. 스폰된 모든 액터를 에디터에서 자동 선택 상태로 만듦
    unreal.EditorLevelLibrary.set_selected_level_actors(spawned_actors)
    unreal.log("심해 유적 스폰 완료!")
    unreal.log("⭐⭐⭐ 팁: 지금 에디터 뷰포트를 클릭하고 'End' 키를 누르시면, 선택된 모든 구조물이 지형(Landscape) 굴곡에 완벽하게 밀착(Snap to Floor)됩니다! ⭐⭐⭐")

if __name__ == '__main__':
    generate_deep_sea_ruins()
