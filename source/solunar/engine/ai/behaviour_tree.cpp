#include "behaviour_tree.h"
#include "tinyxml2.h"

namespace solunar
{
	BehaviourTreeNode::BehaviourTreeNode(const char* pDebugName) : m_update(true), m_id(kBehaviourTreeInvalidBaseType), m_priority(kBehaviourTreeInvalidBaseType), m_parent_id(kBehaviourTreeInvalidBaseType)
	{
#ifdef _DEBUG
		constexpr size_t _kDebugStringMaxLength = sizeof(m_debug_name) / sizeof(m_debug_name[0]);
		std::memcpy(m_debug_name, pDebugName, min(strlen(pDebugName), _kDebugStringMaxLength));
#endif
	}

	BehaviourTreeNode::~BehaviourTreeNode()
	{
	}

	eBehaviourTreeStatus BehaviourTreeNode::Update(World* pWorld, Entity* pOwner, void* pUserStateData, float dt)
	{
		return eBehaviourTreeStatus::kFailure;
	}

	void BehaviourTreeNode::OnEvent(int event_id)
	{
	}

	const char* BehaviourTreeNode::GetName(void) const
	{
#ifdef _DEBUG
		return m_debug_name;
#else
		return nullptr;
#endif
	}

	unsigned char BehaviourTreeNode::GetParentID(void) const
	{
		return m_parent_id;
	}

	void BehaviourTreeNode::SetParentID(unsigned char parent_id)
	{
		m_parent_id = parent_id;
	}

	BehaviourTreeNode** BehaviourTreeNode::GetChildren(void) 
	{
		Assert(false && "provide impl!");
		return nullptr;
	}

	unsigned char BehaviourTreeNode::GetChildrenMaxCount(void) const 
	{
		Assert(false && "provide impl!");
		return kBehaviourTreeInvalidBaseType;
	}

	unsigned char BehaviourTreeNode::GetChildrenCount(void) const
	{
		Assert(false && "provide impl!");
		return kBehaviourTreeInvalidBaseType;
	}

	void BehaviourTreeNode::AddChild(BehaviourTreeNode* pChild)
	{
		Assert(false && "provide impl!");
	}

	void BehaviourTreeNode::SetID(unsigned char id)
	{
		m_id = id;
	}

	unsigned char BehaviourTreeNode::GetID(void) const
	{
		return m_id;
	}

	void BehaviourTreeNode::SetPriority(unsigned char priority)
	{
		m_priority = priority;
	}

	unsigned char BehaviourTreeNode::GetPriority(void) const
	{
		return m_priority;
	}

	bool BehaviourTreeNode::CanUpdate() const
	{
		return m_update;
	}

	void BehaviourTreeNode::SetCanUpdate(bool status)
	{
		m_update = status;
	}
}