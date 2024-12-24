#include "gamepch.h"
#include "weaponcomponent.h"

#include "engine/camera.h"
#include "engine/inputmanager.h"

#include "engine/audio/audiomanager.h"
#include "engine/audio/audiosource.h"

#include "graphics/animatedmodel.h"
#include "graphics/mesh.h"
#include "graphics/ifontmanager.h"
#include "graphics/debugrenderer.h"

#include "shockgame/shockaicomponent.h"
#include "shockgame/demogame.h"

#include <imgui.h>

#include "stb_sprintf.h"

#define ENABLE_TRACE_DEBUG

namespace solunar
{
	namespace tracedbg
	{
		static std::vector< std::pair<glm::vec3, glm::vec3> >	g_debugLines;
		static std::vector< glm::vec3 >							g_debugHits;
		static float											g_debugTime = 0.0f;
	}

	bool g_debugTrace = false;

	IMPLEMENT_OBJECT(WeaponComponent, LogicComponent);
	
	WeaponComponent::WeaponComponent() :
		m_inited(false),
		m_ammo(0),
		m_clipSize(0),
		m_type(WeaponsType::None)
	{
	}

	WeaponComponent::~WeaponComponent()
	{
	}

	void WeaponComponent::RegisterObject()
	{
		g_typeManager->RegisterObject<WeaponComponent>();
	}

	glm::vec3 GetLookingEntityPos(const glm::vec3& rayStart, const glm::vec3& rayEnd)
	{
		RayCastResult rq = {};
		if (Engine::ms_world->RayCast(rq, rayStart, rayEnd))
		{
			Entity* entity = rq.m_entity;
			return rq.m_hitPosition;
		}

		return rayStart;
	}

	void WeaponComponent::Update(float dt)
	{
		if (m_type == WeaponsType::Pistol)
			Update_Pistol(dt);

		if (m_type == WeaponsType::Shotgun)
			Update_Shotgun(dt);

		Update_DEBUG(dt);
	}

	void WeaponComponent::Update_Pistol(float dt)
	{
		static AudioSource* s_fireSound = nullptr;
		static AudioSource* s_reloadSound = nullptr;

#ifdef ENABLE_TRACE_DEBUG
		using namespace tracedbg;
#endif
		AnimatedMeshComponent* mesh = GetEntity()->GetComponent<AnimatedMeshComponent>();
		std::shared_ptr<ModelBase> modelBase = mesh->LockModel();
		AnimatedModel* animatedModel = dynamicCast<AnimatedModel>(modelBase.get());
		if (!m_inited) {
			s_fireSound = AudioManager::GetInstance()->CreateSource("sounds/sfx/weapons/shotgun_fire.wav");
			s_reloadSound = AudioManager::GetInstance()->CreateSource("sounds/sfx/weapons/shotgun_reload.wav");

			m_idleAni = animatedModel->GetAnimationByName("idle");
			m_fireAni = animatedModel->GetAnimationByName("fire");
			m_reload_Ani = animatedModel->GetAnimationByName("reload");

			animatedModel->PlayAnimation(m_idleAni, true);
			m_inited = true;
		}

		Camera* camera = CameraProxy::GetInstance();

		int currentId = animatedModel->GetCurrentAnimationId();
		bool isFireAniFinished = currentId == m_fireAni && animatedModel->IsStoped();
		bool isReloadAniFinished = currentId == m_reload_Ani && animatedModel->IsStoped();
		static bool reload = false;

		const int kMaxAmmo = 12;

		if (InputManager::GetInstance()->IsPressed(KEY_R) && (isFireAniFinished || currentId == m_idleAni) && m_ammo < kMaxAmmo)
		{
			animatedModel->PlayAnimation(m_reload_Ani, false);
		}

		if (isReloadAniFinished)
		{
			m_clipSize -= kMaxAmmo;
			m_ammo = kMaxAmmo;
			animatedModel->PlayAnimation(m_idleAni, true);
		}

		float distance = glm::distance(camera->GetPosition(), GetLookingEntityPos(camera->GetPosition() + camera->GetDirection(),
			camera->GetPosition() + camera->GetDirection() * 1000.0f));
#if 0
		ImGui::GetForegroundDrawList()->AddText(ImVec2(500, 500), 0xff0000ff, std::to_string(distance).c_str());
#endif


		if (InputManager::GetInstance()->IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
			(isFireAniFinished || currentId != m_fireAni) &&
			m_ammo > 0) {
			animatedModel->PlayAnimation(m_fireAni, false);

			if (s_fireSound->IsPlaying())
				s_fireSound->Stop();

			s_fireSound->Play();

			--m_ammo;

				float ra = rand() % 10;
				ra = ra / 100;

				glm::vec3 r;
				r.x = ra * distance;

				ra = rand() % 10;
				ra = ra / 100;
				r.y = ra * distance;

				ra = rand() % 10;
				ra = ra / 100;
				r.z = ra * distance;

				glm::vec3 rayStart = camera->GetPosition() + r + camera->GetDirection();
				glm::vec3 rayEnd = camera->GetPosition() + r + camera->GetDirection() * 1000.0f;

				//g_debugLines.push_back(std::make_pair(rayStart, rayEnd));

				RayCastResult rq = {};
				if (GetWorld()->RayCast(rq, rayStart, rayEnd))
				{
					Entity* entity = rq.m_entity;
					ShockAIComponent* ai = (ShockAIComponent*)entity->GetComponentByTypeInfo(ShockAIComponent::GetStaticTypeInfo());
					if (ai)
						ai->Damage(g_Player, 25.0f);

					Core::Msg("WeaponComponent::Update(): shot entity 0x%p", entity);

#ifdef ENABLE_TRACE_DEBUG
					if (g_debugTrace)
					{
						g_debugLines.push_back(std::make_pair(rayStart, rq.m_hitPosition));
						g_debugHits.push_back(rq.m_hitPosition);
					}
#endif // ENABLE_TRACE_DEBUG
			}
		}

		if (isFireAniFinished) {
			animatedModel->PlayAnimation(m_idleAni, true);
		}

		animatedModel->Update(dt);
	}

	void WeaponComponent::Update_Shotgun(float dt)
	{
		static AudioSource* s_fireSound = nullptr;
		static AudioSource* s_reloadSound = nullptr;

#ifdef ENABLE_TRACE_DEBUG
		using namespace tracedbg;
#endif
		AnimatedMeshComponent* mesh = GetEntity()->GetComponent<AnimatedMeshComponent>();
		std::shared_ptr<ModelBase> modelBase = mesh->LockModel();
		AnimatedModel* animatedModel = dynamicCast<AnimatedModel>(modelBase.get());
		if (!m_inited) {
			s_fireSound = AudioManager::GetInstance()->CreateSource("sounds/sfx/weapons/shotgun_fire.wav");
			s_reloadSound = AudioManager::GetInstance()->CreateSource("sounds/sfx/weapons/shotgun_reload.wav");

			m_idleAni = animatedModel->GetAnimationByName("idle");
			m_fireAni = animatedModel->GetAnimationByName("fire");

			//AnimatedModel: animation prior_to_reload channels 135
			//AnimatedModel : animation reload_last_one channels 135
			//AnimatedModel : animation reload_one channels 135

			m_prior_to_reload_Ani = animatedModel->GetAnimationByName("prior_to_reload");
			m_reload_one_Ani = animatedModel->GetAnimationByName("reload_one");
			m_reload_last_one_Ani = animatedModel->GetAnimationByName("reload_last_one");

			animatedModel->PlayAnimation(m_idleAni, true);
			m_inited = true;
		}

		Camera* camera = CameraProxy::GetInstance();

		int currentId = animatedModel->GetCurrentAnimationId();
		bool isFireAniFinished = currentId == m_fireAni && animatedModel->IsStoped();
		bool isPriorAniFinished = currentId == m_prior_to_reload_Ani && animatedModel->IsStoped();
		bool isReloadAniFinished = currentId == m_reload_one_Ani && animatedModel->IsStoped();
		bool isReloadLastOneAniFinished = currentId == m_reload_last_one_Ani && animatedModel->IsStoped();
		static bool reload = false;

		const int kMaxAmmo = 12;

		if (InputManager::GetInstance()->IsPressed(KEY_R) && (isFireAniFinished || currentId == m_idleAni) && m_ammo < kMaxAmmo)
		{
			animatedModel->PlayAnimation(m_prior_to_reload_Ani, false);
			reload = true;
		}

		if (reload && m_ammo >= kMaxAmmo)
		{
			animatedModel->PlayAnimation(m_reload_last_one_Ani, false);
			if (s_reloadSound->IsPlaying())
				s_reloadSound->Stop();
			reload = false;
		}

		if (isReloadLastOneAniFinished)
			animatedModel->PlayAnimation(m_idleAni, true);


		if (reload)
		{
			if (isPriorAniFinished)
			{
				animatedModel->PlayAnimation(m_reload_one_Ani, false);

				if (!s_reloadSound->IsPlaying())
					s_reloadSound->Play();
			}

			if (isReloadAniFinished)
			{
				m_ammo += 1;
				animatedModel->PlayAnimation(m_reload_one_Ani, false);
				if (!s_reloadSound->IsPlaying())
					s_reloadSound->Play();
			}
		}


		float distance = glm::distance(camera->GetPosition(), GetLookingEntityPos(camera->GetPosition() + camera->GetDirection(),
			camera->GetPosition() + camera->GetDirection() * 1000.0f));
#if 0
		ImGui::GetForegroundDrawList()->AddText(ImVec2(500, 500), 0xff0000ff, std::to_string(distance).c_str());
#endif


		if (InputManager::GetInstance()->IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
			(isFireAniFinished || currentId != m_fireAni) &&
			m_ammo > 0) {
			animatedModel->PlayAnimation(m_fireAni, false);

			if (s_fireSound->IsPlaying())
				s_fireSound->Stop();

			s_fireSound->Play();

			--m_ammo;

			int shellCount = 6;
			for (int i = 0; i < shellCount; i++)
			{


				float ra = rand() % 10;
				ra = ra / 100;

				glm::vec3 r;
				r.x = ra * distance;

				ra = rand() % 10;
				ra = ra / 100;
				r.y = ra * distance;

				ra = rand() % 10;
				ra = ra / 100;
				r.z = ra * distance;

				glm::vec3 rayStart = camera->GetPosition() + r + camera->GetDirection();
				glm::vec3 rayEnd = camera->GetPosition() + r + camera->GetDirection() * 1000.0f;

				//g_debugLines.push_back(std::make_pair(rayStart, rayEnd));

				RayCastResult rq = {};
				if (GetWorld()->RayCast(rq, rayStart, rayEnd))
				{
					Entity* entity = rq.m_entity;
					ShockAIComponent* ai = (ShockAIComponent*)entity->GetComponentByTypeInfo(ShockAIComponent::GetStaticTypeInfo());
					if (ai)
						ai->Damage(g_Player, 25.0f);
					
					Core::Msg("WeaponComponent::Update(): shot entity 0x%p", entity);

#ifdef ENABLE_TRACE_DEBUG
					if (g_debugTrace)
					{
						g_debugLines.push_back(std::make_pair(rayStart, rq.m_hitPosition));
						g_debugHits.push_back(rq.m_hitPosition);
					}
#endif // ENABLE_TRACE_DEBUG
				}
			}
		}

		if (isFireAniFinished) {
			animatedModel->PlayAnimation(m_idleAni, true);
		}

		animatedModel->Update(dt);
	}

	void WeaponComponent::Update_DEBUG(float dt)
	{
#ifdef ENABLE_TRACE_DEBUG
		using namespace tracedbg;

		tracedbg::g_debugTime += dt;

		if (g_debugTrace)
		{
			for (auto& it : g_debugLines)
			{
				g_debugRender.DrawLine(it.first, it.second, glm::vec3(1.0f, 0.0f, 0.0f));
			}

			for (auto& it : g_debugHits)
			{
				g_debugRender.drawAxis(it);
			}

			if (g_debugTime >= 12.0f)
			{
				if (!g_debugLines.empty())
					g_debugLines.clear();

				if (!g_debugHits.empty())
					g_debugHits.clear();

				g_debugTime = 0.0f;
			}
	}
#endif

#if 0
		stbsp_snprintf(s_Buffer, sizeof(s_Buffer), "--- Viewmodel ---");
		g_fontManager->DrawSystemFontShadowed(s_Buffer, (float)view->m_width - 300.0f, 100.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		stbsp_snprintf(s_Buffer, sizeof(s_Buffer), "Animation: %s", animatedModel->GetCurrentAnimation() ? animatedModel->GetCurrentAnimation()->m_name.c_str() : "NO ANIMATION");
		g_fontManager->DrawSystemFontShadowed(s_Buffer, (float)view->m_width - 300.0f, 125.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		stbsp_snprintf(s_Buffer, sizeof(s_Buffer), "Time: %.2f", animatedModel->GetCurrentTime());
		g_fontManager->DrawSystemFontShadowed(s_Buffer, (float)view->m_width - 300.0f, 140.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
#endif
	}

	void WeaponComponent::SetWeaponType(WeaponsType type)
	{
		m_type = type;
		
		if (type == WeaponsType::Pistol)
			m_clipSize = 32;
		else if (type == WeaponsType::Shotgun)
			m_clipSize = 16;
	}
}
