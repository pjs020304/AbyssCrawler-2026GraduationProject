import unreal

def set_prop_safe(obj, prop_name, value):
    """
    UObject 속성을 안전하게 설정하는 헬퍼 함수 (접두사 예외 처리 포함)
    """
    try:
        obj.set_editor_property(prop_name, value)
        return True
    except Exception as e:
        # Boolean 속성의 경우 b_ 접두사가 제외된 이름으로 재시도
        if prop_name.startswith("b_"):
            alt_name = prop_name[2:]
            try:
                obj.set_editor_property(alt_name, value)
                return True
            except Exception:
                pass
        
        # 마지막 수단으로 동적 검색 시도
        return set_property_by_search(obj, [prop_name.replace("b_", "")], value)

def set_property_by_search(obj, search_terms, value):
    """
    검색어 조합을 사용하여 객체의 속성을 찾아 동적으로 설정하는 자가 치유(Self-Healing) 함수
    """
    attributes = dir(obj)
    matched_attrs = []
    for attr in attributes:
        if all(term.lower() in attr.lower() for term in search_terms):
            matched_attrs.append(attr)
            
    if matched_attrs:
        selected_attr = min(matched_attrs, key=len)
        try:
            obj.set_editor_property(selected_attr, value)
            unreal.log(f"Dynamic Match Success: Found and set '{selected_attr}' on '{obj.get_name()}'")
            return True
        except Exception:
            try:
                setattr(obj, selected_attr, value)
                unreal.log(f"Dynamic Match Success (setattr): Found and set '{selected_attr}' on '{obj.get_name()}'")
                return True
            except Exception as e:
                unreal.log_warning(f"Failed to set dynamic property '{selected_attr}' on '{obj.get_name()}': {e}")
    return False

def configure_post_process_settings(settings):
    """
    PostProcessSettings 구조체 제어 함수
    """
    overrides_to_set = {
        "color_saturation": unreal.Vector4(0.4, 0.45, 0.5, 1.0),
        "color_contrast": unreal.Vector4(0.85, 0.85, 0.85, 1.0),
        "auto_exposure_min_brightness": 0.05,
        "auto_exposure_max_brightness": 0.5,
        "auto_exposure_bias": 0.25,
        "scene_fringe_intensity": 1.5
    }
    
    for prop, val in overrides_to_set.items():
        override_names = [f"override_{prop}", f"b_override_{prop}"]
        override_set = False
        for o_name in override_names:
            if hasattr(settings, o_name):
                try:
                    setattr(settings, o_name, True)
                    override_set = True
                    break
                except Exception:
                    pass
        
        if not override_set:
            unreal.log_warning(f"Could not set override flag for post process property: {prop}")
            
        if hasattr(settings, prop):
            try:
                setattr(settings, prop, val)
            except Exception as e:
                unreal.log_warning(f"Failed to set PostProcessSettings value '{prop}': {e}")
        else:
            matched = [attr for attr in dir(settings) if prop.lower() in attr.lower() and not attr.startswith("override") and not attr.startswith("b_override")]
            if matched:
                selected_attr = min(matched, key=len)
                try:
                    setattr(settings, selected_attr, val)
                    unreal.log(f"Dynamic PP Match Success: Set '{selected_attr}' instead of '{prop}'")
                except Exception as e:
                    unreal.log_warning(f"Failed to set dynamic PP property '{selected_attr}': {e}")
            else:
                unreal.log_warning(f"PostProcessSettings has no attribute matching '{prop}'")

def setup_deep_sea_environment():
    # 1. EditorActorSubsystem 가져오기
    editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not editor_actor_subsystem:
        unreal.log_error("Failed to find EditorActorSubsystem. Please ensure the Editor Scripting Utilities plugin is enabled.")
        return

    actors = editor_actor_subsystem.get_all_level_actors()
    unreal.log("Adjusting Deep Sea 3000m Environment for Flashlight Viability...")

    # ==========================================
    # 1. EXPONENTIAL HEIGHT FOG 설정 (손전등 시야 확보를 위한 황금 밸런스 튜닝)
    # ==========================================
    fog_actor = None
    for actor in actors:
        if isinstance(actor, unreal.ExponentialHeightFog):
            fog_actor = actor
            break
            
    if not fog_actor:
        fog_actor = editor_actor_subsystem.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
        fog_actor.set_actor_label("ExponentialHeightFog_DeepSea")
        unreal.log("Spawned new ExponentialHeightFog.")
    else:
        unreal.log("Found existing ExponentialHeightFog.")

    if fog_actor:
        fog_comp = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fog_comp:
            # Volumetric Fog 활성화
            set_prop_safe(fog_comp, "b_enable_volumetric_fog", True)
            
            # [수정] 기본 포그 밀도를 0.13에서 0.05로 추가 하향하여 원거리 가시성 대폭 확보
            set_prop_safe(fog_comp, "fog_density", 0.05)
            
            # 포그 인스캐터링 컬러를 살짝 낮추어 손전등 빛과 어두운 배경의 대비 극대화
            if not set_prop_safe(fog_comp, "fog_inscattering_color", unreal.LinearColor(0.003, 0.015, 0.02, 1.0)):
                set_property_by_search(fog_comp, ["inscattering", "color"], unreal.LinearColor(0.003, 0.015, 0.02, 1.0))
            
            # [수정] Scattering Distribution을 0.72에서 0.55로 추가 완화
            # 전방 산란을 소량 완화하여 플레이어 바로 앞에 "눈부신 포그 장벽(화이트아웃)"이 생기는 현상 방지
            set_prop_safe(fog_comp, "volumetric_fog_scattering_distribution", 0.55)
            
            # [수정] Volumetric Fog Albedo (안개 입자가 반사하는 기본 색상)를 심해의 파란색 톤으로 명시
            # 이를 통해 흰색 손전등 빔이 안개를 통과할 때 자연스럽게 푸른빛을 산란하게 만듭니다.
            albedo_color = unreal.Color(25, 100, 160, 255)
            if not set_prop_safe(fog_comp, "volumetric_fog_albedo", albedo_color):
                set_property_by_search(fog_comp, ["volumetric", "albedo"], albedo_color)
            
            # [수정] Extinction Scale을 1.0에서 0.2로 대폭 하향 조정!!
            # 물 입자에 의한 빛의 소멸 속도를 늦추어 손전등 광선(Spot Light)이 안개를 뚫고 훨씬 멀리까지 도달하도록 만듭니다.
            set_prop_safe(fog_comp, "volumetric_fog_extinction_scale", 0.2)
            
            # [수정] Volumetric Fog Distance를 6000.0에서 10000.0으로 확장
            # 늘어난 가시거리만큼 볼류메트릭 안개 연산 거리를 100m까지 확장합니다.
            set_prop_safe(fog_comp, "volumetric_fog_distance", 10000.0)
            
            unreal.log("Exponential Height Fog configured with Flashlight-friendly settings.")
        else:
            unreal.log_error("Exponential Height Fog Component not found!")

    # ==========================================
    # 2. POST PROCESS VOLUME 설정
    # ==========================================
    pp_actor = None
    for actor in actors:
        if isinstance(actor, unreal.PostProcessVolume):
            pp_actor = actor
            break
            
    if not pp_actor:
        pp_actor = editor_actor_subsystem.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
        pp_actor.set_actor_label("PostProcessVolume_DeepSea")
        unreal.log("Spawned new PostProcessVolume.")
    else:
        unreal.log("Found existing PostProcessVolume.")

    if pp_actor:
        set_prop_safe(pp_actor, "b_unbound", True)
        try:
            settings = pp_actor.get_editor_property("settings")
            configure_post_process_settings(settings)
            pp_actor.set_editor_property("settings", settings)
            unreal.log("Successfully configured Post Process Volume settings.")
        except Exception as e:
            unreal.log_error(f"Failed to configure Post Process Settings: {e}")

    # ==========================================
    # 3. DIRECTIONAL LIGHT 설정
    # ==========================================
    dir_light_actor = None
    for actor in actors:
        if isinstance(actor, unreal.DirectionalLight):
            dir_light_actor = actor
            break
            
    if dir_light_actor:
        light_comp = dir_light_actor.get_component_by_class(unreal.DirectionalLightComponent)
        if light_comp:
            set_prop_safe(light_comp, "intensity", 0.0)
            set_prop_safe(light_comp, "cast_shadows", False)
            unreal.log("Directional Light intensity set to 0.0 and disabled shadows.")
            
    # ==========================================
    # 4. SKY LIGHT 설정
    # ==========================================
    sky_light_actor = None
    for actor in actors:
        if isinstance(actor, unreal.SkyLight):
            sky_light_actor = actor
            break
            
    if sky_light_actor:
        sky_comp = sky_light_actor.get_component_by_class(unreal.SkyLightComponent)
        if sky_comp:
            set_prop_safe(sky_comp, "intensity", 0.02) # 앰비언트 광량을 아주 살짝 더 떨어뜨려 손전등의 존재감 상승
            if not set_prop_safe(sky_comp, "light_color", unreal.Color(6, 24, 32, 255)):
                set_property_by_search(sky_comp, ["light", "color"], unreal.Color(6, 24, 32, 255))
            unreal.log("Sky Light set to dim teal ambient.")

    unreal.log("Deep Sea 3000m Environment setup complete! Please save your level.")

if __name__ == "__main__":
    setup_deep_sea_environment()
