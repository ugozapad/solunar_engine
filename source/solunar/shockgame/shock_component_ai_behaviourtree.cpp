#include "shock_component_ai_behaviourtree.h"
#include "engine/ai/behaviour_tree.h"
#include "shock_ai_behaviour_tree_factory.h"

namespace solunar
{
	constexpr const char* _kAISerializationTag_BehaviourTreeType = "BehaviourTreeType";
	constexpr const char* _kAISerializationAttribute_Value = "value";

	IMPLEMENT_OBJECT(ShockAIBehaviourTree, LogicComponent);

	ShockAIBehaviourTree::ShockAIBehaviourTree() : m_tree_type(eShockBehaviourTree::kUnknown), m_pTree(nullptr)
	{
	}

	ShockAIBehaviourTree::~ShockAIBehaviourTree()
	{
		if (this->m_pTree)
		{
			this->m_pTree->Shutdown();
			delete this->m_pTree;
			this->m_pTree = nullptr;
		}
	}

	void ShockAIBehaviourTree::Update(float dt)
	{
		if (m_pTree)
		{
			m_pTree->Update(dt);
		}
	}

	void ShockAIBehaviourTree::LoadXML(tinyxml2::XMLElement& element)
	{
		World* pWorld = this->GetWorld();

		auto* tag_bt_type = element.FirstChildElement(_kAISerializationTag_BehaviourTreeType);

		if (tag_bt_type)
		{
			auto* attr_value = tag_bt_type->FindAttribute(_kAISerializationAttribute_Value);

			const char* pBehaviourTreeTypeName = attr_value->Value();

			if (pBehaviourTreeTypeName)
			{
				this->m_tree_type = ConvertStringToShockBehaviourTree(pBehaviourTreeTypeName);

				this->m_pTree = CreateBehaviourTree(pWorld, this->GetEntity(), this->m_tree_type);

				Assert(this->m_pTree && "out of memory!");
			}
#ifndef NDEBUG
			else
			{
				Core::Warning("AI: invalid value of behaviour tree type ccan't use behaviour tree factory for creation of BT");
			}
#endif
		}
#ifndef NDEBUG
		else
		{
			Core::Warning("AI: ShockBehaviourTree component presented but type is not, can't use factory for creation!");
		}
#endif
	}

	void ShockAIBehaviourTree::SaveXML(tinyxml2::XMLElement& element)
	{
	}
}