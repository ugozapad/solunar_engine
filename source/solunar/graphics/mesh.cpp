#include "graphicspch.h"
#include "graphics/mesh.h"
#include "graphics/debugrenderer.h"
#include "graphics/renderer.h"
#include "graphics/animatedmodel.h"

#include "engine/entity/world.h"

#define HACK_CLONE_ANIMATEDMODEL

namespace solunar {

	IMPLEMENT_OBJECT(MeshComponent, Component);

	void MeshComponent::RegisterObject()
	{
		g_typeManager->RegisterObject<MeshComponent>();
	}

	MeshComponent::MeshComponent() :
		m_castShadows(true)
	{
	}

	MeshComponent::~MeshComponent()
	{
		m_model.reset();
	}

	void MeshComponent::LoadXML(tinyxml2::XMLElement& element)
	{
		tinyxml2::XMLElement* modelElement = element.FirstChildElement("Model");
		if (modelElement)
		{
			const tinyxml2::XMLAttribute* filenameAttribute = modelElement->FindAttribute("filename");
			if (filenameAttribute && strlen( filenameAttribute->Value())>0)
			{
				m_filename = filenameAttribute->Value();

				if (strstr(m_filename.c_str(), ".model"))
					m_model = g_contentManager->LoadObject<Model>(m_filename);
				else
					m_model = g_contentManager->LoadObject<ModelBase>(m_filename);
			}
			else
			{
				Core::Msg("WARNING: MeshComponent at entity(0x%p) has empty filename attribute in Model element!", GetEntity());
			}
		}

		tinyxml2::XMLElement* castShadowElement = element.FirstChildElement("CastShadow");
		if (castShadowElement)
			castShadowElement->QueryBoolAttribute("value", &m_castShadows);
	}

	void MeshComponent::SaveXML(tinyxml2::XMLElement& element)
	{
		Component::SaveXML(element);

		tinyxml2::XMLElement* modelElement = element.InsertNewChildElement("Model");
		modelElement->SetAttribute("filename", m_filename.c_str());
	}

	void MeshComponent::OnWorldSet(World* world)
	{
		Component::OnWorldSet(world);

		world->GetGraphicsWorld()->AddMesh(this);
	}

	void MeshComponent::OnEntityRemove()
	{
		if (GetWorld())
			GetWorld()->GetGraphicsWorld()->RemoveMesh(this);
	}

	void MeshComponent::Render()
	{
#if 0//ndef FINAL_BUILD
		BoundingBox boundingBox = GetEntity()->GetBoundingBox();
		g_debugRender.drawBoundingBox(boundingBox, glm::vec3(1.f));
#endif // !FINAL_BUILD
	}

	void MeshComponent::LoadModel(const std::string& filename)
	{
		m_filename = filename;

		if (strstr(m_filename.c_str(), ".model"))
			m_model = g_contentManager->LoadObject<Model>(m_filename);
		else
			m_model = g_contentManager->LoadObject<ModelBase>(m_filename);
	}

	std::shared_ptr<ModelBase> MeshComponent::LockModel()
	{
		return m_model.lock();
	}

	std::weak_ptr<ModelBase> MeshComponent::getModel()
	{
		return m_model;
	}

	// animated mesh component

	IMPLEMENT_OBJECT(AnimatedMeshComponent, MeshComponent);

	AnimatedMeshComponent::AnimatedMeshComponent()
	{
	}

	AnimatedMeshComponent::~AnimatedMeshComponent()
	{
		// m_model.reset();
	}

	void AnimatedMeshComponent::LoadXML(tinyxml2::XMLElement& element)
	{
		tinyxml2::XMLElement* modelElement = element.FirstChildElement("Model");
		if (modelElement)
		{
			const tinyxml2::XMLAttribute* filenameAttribute = modelElement->FindAttribute("filename");
			if (filenameAttribute && strlen(filenameAttribute->Value()) > 0)
			{
				m_filename = filenameAttribute->Value();

#ifndef HACK_CLONE_ANIMATEDMODEL
				m_model = g_contentManager->LoadObject<AnimatedModel>(m_filename);
#else
				std::weak_ptr<AnimatedModel> model = g_contentManager->LoadObject<AnimatedModel>(m_filename);
		
				std::shared_ptr<AnimatedModel> modelClone = model.lock()->Clone();

				static int cloneIndex = 0;
				g_contentManager->AddExisting(m_filename + std::to_string(cloneIndex), modelClone);
				cloneIndex++;

				m_model = std::weak_ptr<AnimatedModel>(modelClone);
#endif
				// guarantee cast, trust me 
				std::weak_ptr<AnimatedModel> animatedModel = dynamicCastWeakPtr<AnimatedModel, ModelBase>(m_model);
				

				Entity* entity = GetEntity();
				if (entity)
					entity->SetBoundingBox(animatedModel.lock()->GetBoundingBox()); // #TODO: too shitty usage, lock and unlock on stack :(
			}
			else
			{
				Core::Msg("WARNING: AnimatedMeshComponent at entity(0x%p) has empty filename attribute in Model element!", GetEntity());
			}
		}
	}

	void AnimatedMeshComponent::LoadModel(const std::string& filename)
	{
		if (!filename.empty())
		{
			m_model = g_contentManager->LoadObject<AnimatedModel>(filename);

			// guarantee cast, trust me 
			std::weak_ptr<AnimatedModel> animatedModel = dynamicCastWeakPtr<AnimatedModel, ModelBase>(m_model);

			Entity* entity = GetEntity();
			if (entity)
				entity->SetBoundingBox(animatedModel.lock()->GetBoundingBox()); // #TODO: too shitty usage, lock and unlock on stack :(
		}
		else
		{
			Core::Msg("WARNING: AnimatedMeshComponent at entity(0x%p) has empty filename!", GetEntity());
		}
	}

	void AnimatedMeshComponent::Render()
	{
#if 1
		const BoundingBox& bbox = LockAnimatedModel()->GetBoundingBox();
		GetEntity()->SetBoundingBox(bbox);
#else
		BoundingBox bbox = LockAnimatedModel()->GetBoundingBox();
		bbox.transformAABB(LockAnimatedModel()->GetNodeMatrix(LockAnimatedModel()->GetRootNodeId()));
		GetEntity()->SetBoundingBox(bbox);
#endif

		return;

		if (!m_model.expired())
		{
			// guarantee cast, trust me 
			std::weak_ptr<AnimatedModel> animatedModel = dynamicCastWeakPtr<AnimatedModel, ModelBase>(m_model);
			animatedModel.lock()->DebugRender(GetEntity()->GetWorldTranslation());
		}
	}

	bool AnimatedMeshComponent::HasSkin()
	{
		if (!m_model.expired())
		{
			// guarantee cast, trust me 
			std::weak_ptr<AnimatedModel> animatedModel = dynamicCastWeakPtr<AnimatedModel, ModelBase>(m_model);
			return animatedModel.lock()->HasSkin();
		}

		return false;
	}

	std::shared_ptr<AnimatedModel> AnimatedMeshComponent::LockAnimatedModel()
	{
		std::weak_ptr<AnimatedModel> animatedModel = dynamicCastWeakPtr<AnimatedModel, ModelBase>(m_model);
		return animatedModel.lock();
	}
}