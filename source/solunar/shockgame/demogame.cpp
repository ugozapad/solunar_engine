#include "shockgamepch.h"
#include "shockgame/demogame.h"

#include "core/timer.h"

#include "engine/engine.h"
#include "engine/inputmanager.h"
#include "engine/camera.h"

#include "graphics/ifontmanager.h"
#include "graphics/debugrenderer.h"
#include "graphics/renderer.h"

#include "graphics/light.h"
#include "graphics/mesh.h"

#include <imgui.h>
#include <ImGuizmo.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp> // Matrix decomposition

#include <glm/gtx/spline.hpp>

#include "engine/editor/editor_manager.h"
#include "engine/editor/editor_window_level_inspector.h"
#include "engine/editor/editor_window_entity_editor.h"
#include "engine/editor/editor_window_ai_navigation_builder.h"
#include <shockplayercontroller.h>

namespace solunar
{

IMPLEMENT_OBJECT(UsableAreaComponent, LogicComponent);

BEGIN_PROPERTY_REGISTER(UsableAreaComponent)
{
	REGISTER_PROPERTY(UsableAreaComponent, PropertyString, m_scriptName);
}
END_PROPERTY_REGISTER(UsableAreaComponent)

UsableAreaComponent::UsableAreaComponent()
{
	m_isInited = false;
	m_cost = 0;
}

UsableAreaComponent::~UsableAreaComponent()
{
}

void UsableAreaComponent::LoadXML(tinyxml2::XMLElement& element)
{
	const tinyxml2::XMLElement* scriptElement = element.FirstChildElement("Script");
	if (scriptElement)
	{
		const char* scriptName = nullptr;
		tinyxml2::XMLError error = scriptElement->QueryStringAttribute("value", &scriptName);
		if (error == tinyxml2::XML_SUCCESS)
			m_scriptName = scriptName;
	}

	const tinyxml2::XMLElement* costElement = element.FirstChildElement("Cost");
	if (costElement)
		costElement->QueryIntAttribute("value", &m_cost);
}

void UsableAreaComponent::SaveXML(tinyxml2::XMLElement& element)
{
	tinyxml2::XMLElement* scriptElement = element.InsertNewChildElement("Script");
	scriptElement->SetAttribute("value", m_scriptName.c_str());

	tinyxml2::XMLElement* costElement = element.InsertNewChildElement("Script");
	costElement->SetAttribute("value", m_cost);
}

void UsableAreaComponent::OnInit()
{
}

void UsableAreaComponent::Update(float delta)
{
	if (!m_isInited)
	{
		OnInit();

		m_isInited = true;
	}

#if 0
	static char buf[256];
	snprintf(buf, sizeof(buf), "UsableAreaComponent: Script: %s", m_scriptName.c_str());

	Debug_Draw3DText(buf, GetEntity()->GetWorldPosition(), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
#endif
}

void Debug_Draw3DText(const char* text, const glm::vec3& position, const glm::vec4& color, float y /*= 0.0f*/)
{
	View* view = CameraProxy::GetInstance()->GetView();

	glm::vec4 vp = glm::vec4(0.0f, 0.0f, (float)view->m_width, (float)view->m_height);
	glm::vec3 proj = glm::project(position, view->m_view, view->m_projection, vp);
	if (proj.z >= 1.0f)
		return; // clip

	proj.y = ((float)view->m_height - 1.0f - proj.y) + y;

	g_fontManager->DrawSystemFontShadowed(text, proj.x, proj.y, color);
}

static GameManager s_GameManager;
GameManager* g_GameManager = &s_GameManager;

GameManager::GameManager()
{
}

GameManager::~GameManager()
{
}

void GameManager::OnWorldLoad(const std::string& worldName, World* pLoadedWorld)
{
	Core::Msg("GameManager: Initializing");

	Assert(pLoadedWorld && "expected to be always a valid pointer!");

	if (pLoadedWorld)
	{
		// todo: kirrik -> add ability to add preprocessor from cmake like -DENABLE_EDITOR=ON and in IDE we use like #ifdef SOLUNAR_EDITOR
		if (g_engineData.m_editor)
		{
			g_editorManager->RegisterWindow(mem_new<EditorWindow_LevelInspector>(EditorWindow_LevelInspector()));
			g_editorManager->RegisterWindow(mem_new<EditorWindow_EntityEditor>(EditorWindow_EntityEditor()));
			g_editorManager->RegisterWindow(mem_new<EditorWindow_AINavigationBuilder>(EditorWindow_AINavigationBuilder()));
		
			// add editor player to the world
			Entity* editorEntity = pLoadedWorld->CreateEntity();
			editorEntity->CreateComponent<EditorCameraComponent>();
		}
		else
		{
			// level manager
			g_ShelterLevelManager = pLoadedWorld->CreateEntity()->CreateComponent<ShelterLevelManagerComponent>();

			// player creation

			std::vector<Entity*> playerEntities = pLoadedWorld->GetEntityManager().GetEntitiesWithComponent<ShockPlayerController>();
			if (!playerEntities.empty())
			{
				Core::Msg("GameManager::OnWorldLoad: World '%s' already have player. Please replace player with his spawner (player spawner).",
					worldName.c_str());

				return;
			}

			std::vector<Entity*> spawnerEntities = pLoadedWorld->GetEntityManager().GetEntitiesWithComponent<PlayerSpawnComponent>();
			if (spawnerEntities.empty())
			{
				Core::Msg("GameManager::OnWorldLoad: World '%s' doesn't have player spawner. Canno't spawn player!",
					worldName.c_str());

				return;
			}

			Entity* player = pLoadedWorld->CreateEntity();
			player->SetPosition(spawnerEntities[0]->GetWorldPosition());
			player->CreateComponent<ShockPlayerController>();

		}
	}
}

Entity* g_Player = nullptr;

Entity* g_SelectedEntity = nullptr;

IMPLEMENT_OBJECT(EditorCameraComponent, LogicComponent);

EditorCameraComponent::EditorCameraComponent() :
	m_camera(nullptr)
{
}

EditorCameraComponent::~EditorCameraComponent()
{
}

void EditorCameraComponent::OnEntitySet(Entity* entity)
{
	LogicComponent::OnEntitySet(entity);

	m_camera = entity->CreateComponent<CameraFirstPersonComponent>();
	
	CameraProxy::GetInstance()->SetCameraComponent(m_camera);

	g_engineData.m_shouldCaptureMouse = false;
	g_engineData.m_shouldHideMouse = false;
}

void EditorCameraComponent::OnEntityRemove()
{
	LogicComponent::OnEntityRemove();
}

inline glm::vec3 Glm_GetPositionFromMat4(const glm::mat4& m)
{
	glm::vec3 scale = glm::vec3(0.0f);
	glm::quat orientation = glm::identity<glm::quat>();
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 skew = glm::vec3(0.0f);
	glm::vec4 persp = glm::vec4(0.0f);
	glm::decompose(m, scale, orientation, position, skew, persp);
	return position;
}

void DrawProperties(const std::vector<IProperty*>& properties, Object* object)
{
	for (int i = 0; i < properties.size(); i++)
	{
		IProperty* property = properties[i];
		if (property->GetType() == PropertyType_Float)
		{
			float value;
			PropertyGetValue(object, property, value);
			ImGui::DragFloat(property->GetName(), &value);
		}
		if (property->GetType() == PropertyType_Vector3)
		{
			glm::vec3 value;
			PropertyGetValue(object, property, value);
			ImGui::DragFloat3(property->GetName(), &value[0]);
		}
		if (property->GetType() == PropertyType_Vector4)
		{
			glm::vec4 value;
			PropertyGetValue(object, property, value);
			ImGui::DragFloat4(property->GetName(), &value[0]);
		}
		if (property->GetType() == PropertyType_Quaternion)
		{
			glm::quat value;
			PropertyGetValue(object, property, value);
			ImGui::DragFloat4(property->GetName(), &value[0]);
		}
		if (property->GetType() == PropertyType_String)
		{
			std::string value;
			PropertyGetValue(object, property, value);
			ImGui::InputText(property->GetName(), (char*)value.c_str(), value.size() + 1);
		}
		if (property->GetType() == PropertyType_Bool)
		{
			bool value;
			PropertyGetValue(object, property, value);
			ImGui::Checkbox(property->GetName(), &value);
		}
	}
}

void DrawEntityPropertyWindow(Entity* entity)
{
	if (ImGui::Begin("Entity Properties"))
	{
		if (entity)
		{
			ImGui::Text("%s", entity->GetTypeInfo()->GetClassName());
			std::vector<IProperty*> properties;
			PropertyManager::GetInstance()->GetProperties(entity->GetTypeInfo(), properties);
			DrawProperties(properties, entity);

			PointLightComponent* pl = entity->GetComponent<PointLightComponent>();
			if (pl)
			{
				ImGui::DragFloat("RADIUS", &pl->m_radius);
			}

			// components
			const std::vector<Component*>& components = entity->GetAllComponents();
			for (int i = 0; i < components.size(); i++)
			{
				ImGui::Separator();
				ImGui::Text("%s", components[i]->GetTypeInfo()->GetClassName());

				properties.clear();

				PropertyManager::GetInstance()->GetProperties(components[i]->GetTypeInfo(), properties);

				DrawProperties(properties, components[i]);
			}
		}
	}

	ImGui::End();
}

void LevelInspector()
{
#if 0
	static int selection = 0;
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
	const std::vector<Entity*>& entities = Engine::ms_world->GetEntityManager().GetEntities();
	if (ImGui::Begin("Entity Properties"))
	{
		if (ImGui::BeginTable("table_scrolly", 1, flags))
		{
			ImGui::TableSetupScrollEditorze(0, 1); // Make top row always visible
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
			ImGui::TableHeadersRow();

			for (int i = 0;i < entities.size();i++)
			{
				Entity* entity = entities[i];

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				bool isLight = !!it->GetComponent<LightComponent>();
				if (isLight)
					ImGui::Text("Light 0x%p", it);
				else
					ImGui::Text("Entity 0x%p", it);
			}

			ImGui::EndTable();
		}
	}

	ImGui::End();
#endif

}

void EditorCameraComponent::Update(float delta)
{
	static std::vector<std::pair<glm::vec3, glm::vec3>> debugRays;

	LogicComponent::Update(delta);

	LevelInspector();

	//if (!g_SelectedEntity)
	//{
	//	std::vector<Entity*> lights = Engine::ms_world->GetEntityManager().GetEntitiesWithComponent<PointLightComponent>();
	//	if (!lights.empty())
	//		g_SelectedEntity = lights[0];
	//}

	InputManager* input = InputManager::GetInstance();
	if (input->IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		g_engineData.m_shouldCaptureMouse = true;
		g_engineData.m_shouldHideMouse = true;

		glm::vec2 mousePos = input->GetCursorPos();

		glm::vec2 deltaMousePos = input->GetDeltaCursorPos();
		m_camera->updateFromMousePosition(deltaMousePos);

		glm::vec3 cameraDirection = CameraProxy::GetInstance()->GetDirection();
		glm::vec3 pos = GetEntity()->GetPosition();
		float camSpeed = 8.0f * delta;

		if (input->IsPressed(KeyboardKeys::KEY_LEFT_SHIFT))
			camSpeed = 18.0f * delta;

		if (input->IsPressed(KeyboardKeys::KEY_W))
			pos += camSpeed * cameraDirection;
		if (input->IsPressed(KeyboardKeys::KEY_S))
			pos -= camSpeed * cameraDirection;

		if (input->IsPressed(KeyboardKeys::KEY_Q))
			pos -= glm::normalize(glm::cross(cameraDirection, glm::vec3(1.0f, 0.0f, 0.0f))) * camSpeed;
		if (input->IsPressed(KeyboardKeys::KEY_E))
			pos += glm::normalize(glm::cross(cameraDirection, glm::vec3(1.0f, 0.0f, 0.0f))) * camSpeed;

		if (input->IsPressed(KeyboardKeys::KEY_A))
			pos -= glm::normalize(glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * camSpeed;
		if (input->IsPressed(KeyboardKeys::KEY_D))
			pos += glm::normalize(glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * camSpeed;

		GetEntity()->SetPosition(pos);
	
		// ray pick

		// no, not handling it here
		if (false && input->IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			Camera* camera = CameraProxy::GetInstance();
			glm::vec3 rayBegin = camera->GetPosition() + camera->GetDirection();
			glm::vec3 rayEnd = camera->GetPosition() + (camera->GetDirection() * 100.0f);
			RayCastResult rayCastResult;
			debugRays.emplace_back(std::make_pair(rayBegin, rayEnd));
			if (GetWorld()->RayCast(rayCastResult, rayBegin, rayEnd))
			{
				g_SelectedEntity = rayCastResult.m_entity;
			}
			else
			{
				g_SelectedEntity = nullptr;
			}
		}
	}
	else
	{
		g_engineData.m_shouldCaptureMouse = false;
		g_engineData.m_shouldHideMouse = false;

#if 0
		// ray pick
		
		if (input->IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			glm::vec2 pos = input->GetCursorPos();
			glm::vec3 rayBegin = CameraProxy::GetInstance()->GetScreenRay(pos.x, pos.y);
			glm::vec3 rayEnd = rayBegin * -100.0f;
			RayCastResult rayCastResult;

			//debugRays.emplace_back(std::make_pair(rayBegin, rayEnd));

			if (GetWorld()->RayCast(rayCastResult, rayBegin, rayEnd))
			{
				g_SelectedEntity = rayCastResult.m_entity;
			}
			else
			{
				g_SelectedEntity = nullptr;
			}
		}
#endif
	}

#if 0
	for (int i = 0; i < debugRays.size(); i++)
	{
		g_debugRender.DrawLine(debugRays[i].first, debugRays[i].second, glm::vec3(1.0f, 0.0f, 0.0f));
	}
#endif

	if (g_SelectedEntity)
	{
		//g_debugRender.drawBoundingBox(g_SelectedEntity->GetBoundingBox(), glm::vec3(0.0f, 0.0f, 1.0f));

		{
			char buf[256];
			snprintf(buf, sizeof(buf), "Entity: 0x%p", g_SelectedEntity);
			Debug_Draw3DText(buf, g_SelectedEntity->GetWorldPosition(), glm::vec4(1.0f), -25.0f);

			bool isLight = !!g_SelectedEntity->GetComponent<LightComponent>();
			bool isMesh = !!g_SelectedEntity->GetComponent<MeshComponent>();

			if (isLight)
			{
				snprintf(buf, sizeof(buf), "Light");
				Debug_Draw3DText(buf, g_SelectedEntity->GetWorldPosition(), glm::vec4(1.0f), -50.0f);

				BoundingBox bbox = g_SelectedEntity->GetBoundingBox();
				g_debugRender.drawBoundingBox(bbox, glm::vec3(1.0f, 0.5f, 0.0f));
			}

			if (isMesh)
			{
				snprintf(buf, sizeof(buf), "Mesh");
				Debug_Draw3DText(buf, g_SelectedEntity->GetWorldPosition(), glm::vec4(1.0f), -50.0f);
			}
		}

		static glm::mat4 objectMatrix = g_SelectedEntity->GetWorldTranslation();
		
		View* view = CameraProxy::GetInstance()->GetView();

		glm::mat4 cameraView = view->m_view;
		glm::mat4 cameraProj = view->m_projection;

		static int g_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
		static int g_CurrentGizmoMode = ImGuizmo::WORLD;
		
		if (ImGuizmo::Manipulate(
			&cameraView[0][0],
			&cameraProj[0][0],
			(ImGuizmo::OPERATION)g_CurrentGizmoOperation,
			(ImGuizmo::MODE)g_CurrentGizmoMode,
			&objectMatrix[0][0]))
		{
			glm::vec3 pos = Glm_GetPositionFromMat4(objectMatrix);
			g_SelectedEntity->SetPosition(pos);

		}

	}

	DrawEntityPropertyWindow(g_SelectedEntity);

#if 0
	if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(1))
		ImGui::OpenPopup("MyPopup");
	if (ImGui::BeginPopup("MyPopup"))
	{
			ImGui::Text("Hello, world!");
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Free Camera
IMPLEMENT_OBJECT(FreeCameraComponent, LogicComponent);

FreeCameraComponent::FreeCameraComponent() :
	m_camera(nullptr)
{
}

FreeCameraComponent::~FreeCameraComponent()
{
}

void FreeCameraComponent::OnEntitySet(Entity* entity)
{
	LogicComponent::OnEntitySet(entity);

	m_camera = entity->CreateComponent<CameraFirstPersonComponent>();

	CameraProxy::GetInstance()->SetCameraComponent(m_camera);

	g_engineData.m_shouldCaptureMouse = true;
	g_engineData.m_shouldHideMouse = true;
}

void FreeCameraComponent::OnEntityRemove()
{
	LogicComponent::OnEntityRemove();
}

void FreeCameraComponent::Update(float delta)
{
	LogicComponent::Update(delta);

	InputManager* input = InputManager::GetInstance();

	glm::vec2 mousePos = input->GetCursorPos();

	glm::vec2 deltaMousePos = input->GetDeltaCursorPos();
	m_camera->updateFromMousePosition(deltaMousePos);

	glm::vec3 cameraDirection = CameraProxy::GetInstance()->GetDirection();
	glm::vec3 pos = GetEntity()->GetPosition();
	float camSpeed = 8.0f * delta;

	if (input->IsPressed(KeyboardKeys::KEY_LEFT_SHIFT))
		camSpeed = 18.0f * delta;

	if (input->IsPressed(KeyboardKeys::KEY_W))
		pos += camSpeed * cameraDirection;
	if (input->IsPressed(KeyboardKeys::KEY_S))
		pos -= camSpeed * cameraDirection;

	if (input->IsPressed(KeyboardKeys::KEY_Q))
		pos -= glm::normalize(glm::cross(cameraDirection, glm::vec3(1.0f, 0.0f, 0.0f))) * camSpeed;
	if (input->IsPressed(KeyboardKeys::KEY_E))
		pos += glm::normalize(glm::cross(cameraDirection, glm::vec3(1.0f, 0.0f, 0.0f))) * camSpeed;

	if (input->IsPressed(KeyboardKeys::KEY_A))
		pos -= glm::normalize(glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * camSpeed;
	if (input->IsPressed(KeyboardKeys::KEY_D))
		pos += glm::normalize(glm::cross(cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f))) * camSpeed;

	GetEntity()->SetPosition(pos);

	if (InputManager::GetInstance()->IsPressed(KEY_F1))
		g_renderer->SetRenderMode(RendererViewMode::Wireframe);
	else if (InputManager::GetInstance()->IsPressed(KEY_F2))
		g_renderer->SetRenderMode(RendererViewMode::Lit);
}

FadeRenderer* FadeRenderer::GetInstance()
{
	static FadeRenderer s_FadeRenderer;
	return &s_FadeRenderer;
}

void FadeRenderer::SetFade(float time, bool isOut)
{
	m_currentTime = isOut ? time : 0.0f;
	m_time = time;
	m_isOut = isOut;
}

void FadeRenderer::Draw()
{
	if (m_isOut)
	{
		if (m_currentTime>=0.0001f)
			m_currentTime -= Timer::GetInstance()->GetDelta();
	}
	else
	{
		if (m_currentTime <= m_time)
			m_currentTime += Timer::GetInstance()->GetDelta();
	}

	// don't draw on reach time * 2
	if (m_currentTime >= m_time * 2.0f)
		return;

	ImGuiIO& io = ImGui::GetIO();

	float a = m_currentTime / m_time;
	ImU32 color = IM_COL32(0, 0, 0, a * 255);

	ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0.0f, 0.0f), io.DisplaySize, color);
}

TitleRenderer* TitleRenderer::GetInstance()
{
	static TitleRenderer s_TitleRenderer;
	return &s_TitleRenderer;
}

void TitleRenderer::SetTitle(const std::string& name, float time, bool isOut)
{
	m_currentTime = isOut ? time : 0.0f;
	m_time = time;
	m_isOut = isOut;
	m_name = name;
}

void TitleRenderer::Draw()
{
	if (m_isOut)
	{
		if (m_currentTime >= 0.0001f)
			m_currentTime -= Timer::GetInstance()->GetDelta();
	}
	else
	{
		if (m_currentTime <= m_time)
			m_currentTime += Timer::GetInstance()->GetDelta();
	}

	// don't draw on reach time * 2
	if (m_currentTime >= m_time * 2.0f)
		return;

	float a = m_currentTime / m_time;

	View* view = CameraProxy::GetInstance()->GetView();

	static IFont* font = g_fontManager->CreateFont("textures/ui/Anton-Regular.ttf", 86.0f);
	font->DrawText(m_name.c_str(), view->m_width / 2.0f - 220.0f, view->m_height / 2.0f, glm::vec4(1.0f, 0.1f, 0.1f, a));
}

IMPLEMENT_OBJECT(PlayerSpawnComponent, Component);

PlayerSpawnComponent::PlayerSpawnComponent()
{
}

PlayerSpawnComponent::~PlayerSpawnComponent()
{
}

IMPLEMENT_OBJECT(DoorCoverComponent, LogicComponent);

DoorCoverComponent::DoorCoverComponent()
{
	m_time = 0.0f;
}

DoorCoverComponent::~DoorCoverComponent()
{
}

void DoorCoverComponent::Update(float dt)
{
	const float endtime = 12.0f;

	float d = m_time / endtime;
	m_time += dt;

	glm::vec3 sp = glm::lerp(GetEntity()->GetPosition(), m_pointPosition, d*0.002f);  // glm::spline()

	GetEntity()->SetPosition(sp);
}

void DoorCoverComponent::SetPointPosition(const glm::vec3& position)
{
	m_pointPosition = position;
}

ShelterLevelManagerComponent* g_ShelterLevelManager = nullptr;

IMPLEMENT_OBJECT(ShelterLevelManagerComponent, LogicComponent);

ShelterLevelManagerComponent::ShelterLevelManagerComponent()
{
	m_barricadePoints[0] = nullptr;
	m_barricadePoints[1] = nullptr;
	m_barricadePoints[2] = nullptr;

	for (int i = 0; i < kDoorCount; i++)
	{
		m_barricades[i][0] = nullptr;
		m_barricades[i][1] = nullptr;
		m_barricades[i][2] = nullptr;
		m_barricades[i][3] = nullptr;
		m_barricades[i][4] = nullptr;
	}
}

ShelterLevelManagerComponent::~ShelterLevelManagerComponent()
{
}

void ShelterLevelManagerComponent::OnInit()
{
#ifndef _DEBUG
	// initialize fade
	FadeRenderer::GetInstance()->SetFade(4.0f, true);

	// initialize title
	TitleRenderer::GetInstance()->SetTitle("Survive the 4 waves", 8.0f, true);
#endif // !_DEBUG

	// find barricade points
	m_barricadePoints[0] = GetWorld()->GetEntityManager().GetEntityByName("door1_barricade_point");
	m_barricadePoints[1] = GetWorld()->GetEntityManager().GetEntityByName("door2_barricade_point");
	m_barricadePoints[2] = GetWorld()->GetEntityManager().GetEntityByName("door3_barricade_point");

	if (!m_barricadePoints[0] || m_barricadePoints[1] || m_barricadePoints[2])
	{
		Core::Msg("ERROR: world doesn't consist entities for barricade point. i.g. door1_barricade_point");
		return;
	}

	// initialize models
	for (int i = 0; i < kDoorCount; i++)
	{
		Entity* barricadePoint = m_barricadePoints[i];

		for (int j = 0; j < kBarricadeCount; j++)
		{
			m_barricades[i][j] = GetWorld()->CreateEntity();

			m_barricades[i][j]->SetPosition(barricadePoint->GetPosition() + (float)j * 8.25f);

			DoorCoverComponent* doorCoverComponent = m_barricades[i][j]->CreateComponent<DoorCoverComponent>();
			doorCoverComponent->SetPointPosition(barricadePoint->GetPosition());

			MeshComponent* mesh = m_barricades[i][j]->CreateComponent<MeshComponent>();
			mesh->LoadModel("models/prop_wooden_crate.model");
		}
	}
}

void ShelterLevelManagerComponent::OnEntitySet(Entity* entity)
{
	Component::OnEntitySet(entity);

	entity->SetName("unnamed level manager entity");

	g_ShelterLevelManager = this;
}

void ShelterLevelManagerComponent::OnEntityRemove()
{
	g_ShelterLevelManager = nullptr;
}

void ShelterLevelManagerComponent::Update(float dt)
{
#ifndef _DEBUG
	// draw fade
	FadeRenderer::GetInstance()->Draw();

	// draw title
	TitleRenderer::GetInstance()->Draw();
#endif // !_DEBUG
}

}
