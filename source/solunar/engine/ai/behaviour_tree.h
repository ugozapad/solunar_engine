// -----------------------------------------
//
//   author: wh1t3lord (https://github.com/wh1t3lord)
//	 description: BehaviourTree implementation (embedded like approach) 
//
// -----------------------------------------


#ifndef BEHAVIOUR_TREE_H
#define BEHAVIOUR_TREE_H

namespace tinyxml2
{
	class XMLElement;
}


namespace solunar
{
	class World;
}

namespace solunar
{
	enum eBehaviourTreeStatus
	{
		kFailure,
		kRunning,
		kSuccess
	};

	// user passes look-up table with timings for each behaviour tree, it has "default array" if user didn't specify anything
	enum eBehaviourTreeNodePriority
	{
		// rarely updates
		kLow,
		// not often updates
		kMedium,
		// always updates
		kHigh,
		kSize
	};

	constexpr unsigned char kBehaviourTreeMaxCountOfChildren = 8;
	constexpr unsigned char kBehaviourTreeInvalidBaseType = unsigned char(-1);

	class BehaviourTreeNode
	{
	public:
		BehaviourTreeNode(const char* pDebugName);
		virtual ~BehaviourTreeNode();

		virtual eBehaviourTreeStatus Update(World* pWorld, Entity* pOwner, void* pUserStateData, float dt);
		virtual void OnEvent(int event_id);
		virtual BehaviourTreeNode** GetChildren(void);
		virtual unsigned char GetChildrenMaxCount(void) const;
		virtual unsigned char GetChildrenCount(void) const;
		virtual void AddChild(BehaviourTreeNode* pChild);

		void SetID(unsigned char id);
		unsigned char GetID(void) const;

		void SetPriority(unsigned char priority);
		unsigned char GetPriority(void) const;

		bool CanUpdate() const;
		void SetCanUpdate(bool status);

		const char* GetName(void) const;

		unsigned char GetParentID(void) const;
		void SetParentID(unsigned char parent_id);

	private:
		// in case of linear allocator we can't delete node
		bool m_update;
		// allocated id from allocator
		unsigned char m_id;
		unsigned char m_priority;
		unsigned char m_parent_id;
#ifdef _DEBUG
		char m_debug_name[32];
#endif
	};

	class IBehaviourTreeAllocator
	{
	public:
		virtual ~IBehaviourTreeAllocator() {}

		virtual void* AllocateNode(size_t size_of_class) = 0;
		virtual void DeleteNode(void* pNode) = 0;
		virtual void Reset(void) = 0;
	};

	class IBehaviourTree
	{
	public:
		virtual ~IBehaviourTree() {}
		virtual void Update(float dt) = 0;
		virtual void Shutdown() = 0;
	};

	// choose size per user behaviour tree implementation, it might be very different!
	// todo: wh1t3lord -> provide implementation of allocator for case where node deletes and user can make behaviour tree dynamic like in runtime adds new nodes and etc
	template<unsigned char MaxNumberOfNodes, size_t MaxSizeOfBehaviourTreeNodeInBytes>
	class BehaviourTreeLinearAllocator : public IBehaviourTreeAllocator
	{
	public:
		BehaviourTreeLinearAllocator() :IBehaviourTreeAllocator(), m_allocated_nodes_count(0) {}
		~BehaviourTreeLinearAllocator() {}

		// use this for placement new
		// e.g.: new (pOffsetFromAllocateNodePointer) BehaviourTreeNodeXXX()
		void* AllocateNode(size_t size_of_class) override
		{
			Assert(m_allocated_nodes_count < MaxNumberOfNodes && "overflow you need to shrink your allocator");
			Assert(size_of_class <= MaxSizeOfBehaviourTreeNodeInBytes && "can't allocate memory for a such data! You specified not the highest class that might use in your tree");

			void* pResult = nullptr;

			if (size_of_class <= MaxSizeOfBehaviourTreeNodeInBytes)
			{
				pResult = &m_memory_buffer[m_allocated_nodes_count * MaxSizeOfBehaviourTreeNodeInBytes];
			}

			++m_allocated_nodes_count;

			return pResult;
		}

		void DeleteNode(void* pNode) override
		{
			// todo: wh1t3lord -> think about it carefully and finish it, because when we try to delete from parent some of it child it can break the runtime execution
			// Linear it is not supposed to rebuild buffer or something
			BehaviourTreeNode* pCasted = static_cast<BehaviourTreeNode*>(pNode);

			if (pCasted)
			{
				pCasted->SetCanUpdate(false);
				pCasted->~BehaviourTreeNode();
			}
		}

		void Reset(void) override
		{
			m_allocated_nodes_count = 0;
			std::memset(m_memory_buffer, 0, sizeof(m_memory_buffer));
		}

		constexpr static unsigned char GetMaxNodes() {
			return MaxNumberOfNodes;
		}

	private:
		unsigned char m_allocated_nodes_count;
		unsigned char m_memory_buffer[MaxNumberOfNodes * MaxSizeOfBehaviourTreeNodeInBytes];
	};

	// low priority - updates after 1 second; medium priority - updates after half of second; high priority - updates immedeatily without pauses;
	const float kBehaviourTreeDefaultUserPriorityTimings[eBehaviourTreeNodePriority::kSize] = { 1.0f, 0.5f, 0.0f };

	/*
		How to work with this?

		You should specify allocator, for now there's only one allocator is linear, for future you can develop an allocator for general-purpose use and make behaviour tree fully dynamic and change nodes at runtime (it might be useful)

		For making ultra fast performant implementation allocator provides new placement policy of allocating nodes. Because we can know at compile time how many nodes will BT use and how REAL memory needs to handle a such tree. It provides best optimization between memory consumption/performance.

		Just add forever node and you get a node that is always updates (every frame)

		Then add some node that you want:

			1. Selector node
				Updates until some of nodes return success

			2. Sequence node
				Updates until some of nodes return failure

			3. Forever node
				Always updates no matter of status

			4. Once
				Once updates then removes from tree

			5.

		Node that stopped updates it means that a node was deleted from a tree, depends on allocator
	*/
	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	class BehaviourTree : public IBehaviourTree
	{
		static_assert(Allocator::GetMaxNodes() >= MaxNodesInTree && "suppose that allocator can allocate at least MaxNodeInTree! Sadly but in C++ we can't obtain template parameter from class :(");
	public:
		BehaviourTree(const char* pDebugName);
		~BehaviourTree();

		void Init(World* pLoadedWorld, Entity* pOwner, float p_user_priority_timings[eBehaviourTreeNodePriority::kSize] = 0);
		void Update(float dt) override;
		void Shutdown() override;

		const char* GetName(void) const;

		void GetAllNodes(BehaviourTreeNode**& pNodes, unsigned char& count);
		BehaviourTreeNode* GetNode(unsigned char id) const;
		unsigned char GetMaxNodesCount(void) const;
		unsigned char GetCurrentNodesCount(void) const;

		// add node to tree and returns the created instance of user node type
		// if there's no memory returns nullptr (can't allocate because no space or size of class was not expected by instantieted allocator you will get assert about class size)
		template<typename UserBehaviourTreeNodeType>
		UserBehaviourTreeNodeType* AddNode(const char* pNodeDebugName);

		template<typename UserBehaviourTreeNodeType>
		UserBehaviourTreeNodeType* AddNode(unsigned char parent_id, const char* pNodeDebugName);

		void* GetUserData(void) { return static_cast<void*>(&this->m_user_data); }
		const void* GetUserData(void) const { return static_cast<const void*>(&this->m_user_data); }

		void Dump(const char* pStackBuffer);
		void PrintLogToConsole();
		void PrintLogToIDEConsole();

	private:
		// moves all root nodes to first places of array in order to make correct tree execution, otherwise other nodes can be called but node handles them (double execution)
		void RebuildOrder();

	private:
#ifdef _DEBUG
		bool m_shutdown_was_called;
		bool m_init_was_called;
#endif
		unsigned char m_current_nodes_count;
		World* m_pWorld;
		Entity* m_pOwner;
		float m_priorities[eBehaviourTreeNodePriority::kSize];
#ifdef _DEBUG
		char m_debug_name[16];
#endif
		BehaviourTreeNode* m_pNodes[MaxNodesInTree];

		// stores temp information for handling logic, like what target we found 
		// what we need to do and etc, it passes among ALL nodes for exchange/interaction between nodes instead making some global manager we just use structs (or classes)
		// on user side what we need really for storing information for each monster/ai creature so we defines memory what we really need without making global manager thing
		UserLogicDataType m_user_data;

		Allocator m_allocator;
	};

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::BehaviourTree(const char* pDebugName) :
#ifdef _DEBUG
		m_shutdown_was_called(false),
		m_init_was_called(false),
#endif
		m_current_nodes_count(0), m_pWorld(nullptr), m_pOwner(nullptr), m_priorities{-1.0f,-1.0f,-1.0f}
	{
		static_assert(std::is_pod<UserLogicDataType>::value && "it must be plain class or struct (preferrably struct without set/get methods)");
		std::memset(&this->m_user_data, 0, sizeof(this->m_user_data));

#ifdef _DEBUG
		constexpr size_t _kDebugNameStringMaxLength = sizeof(m_debug_name) / sizeof(m_debug_name[0]);
		std::memcpy(m_debug_name, pDebugName, min(strlen(pDebugName), _kDebugNameStringMaxLength));
#endif
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::~BehaviourTree()
	{
#ifdef _DEBUG
		Assert(m_shutdown_was_called && "you forgot to call ::Shutdown, it is important we must issue ~dtor for user nodes because of placement new policy");
#endif
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline void BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::Init(World* pWorld, Entity* pOwner, float p_user_priority_timings[eBehaviourTreeNodePriority::kSize])
	{
		Assert(pWorld && "must be valid!");
		Assert(pOwner && "must be valid!");

#ifdef _DEBUG
		m_init_was_called = true;
#endif

		this->m_pWorld = pWorld;
		this->m_pOwner = pOwner;

		if (p_user_priority_timings)
		{
			std::memcpy(m_priorities, p_user_priority_timings, sizeof(m_priorities));
		}
		else
		{
			m_priorities[eBehaviourTreeNodePriority::kLow] = 1.0f;
			m_priorities[eBehaviourTreeNodePriority::kMedium] = 0.35f;
			m_priorities[eBehaviourTreeNodePriority::kHigh] = 0.0f;
		}
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline void BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::Update(float dt)
	{
#ifdef _DEBUG
		Assert(m_priorities[eBehaviourTreeNodePriority::kLow] >= 0.0f && "you forgot to call ::Init method!");
		Assert(m_init_was_called && "must be called ::Init before updating");
#endif

		for (unsigned char i = 0; i < m_current_nodes_count; ++i)
		{
			BehaviourTreeNode* pNode = m_pNodes[i];

			Assert(pNode && "must be valid!");

			if (pNode)
			{
				if (pNode->GetParentID() == kBehaviourTreeInvalidBaseType)
				{
					if (pNode->CanUpdate())
					{
						pNode->Update(this->m_pWorld, this->m_pOwner, static_cast<void*>(&this->m_user_data), dt);
					}
				}
				else
				{
					break;
				}
			}
		}
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline void BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::Shutdown()
	{
		for (unsigned char i = 0; i < m_current_nodes_count; ++i)
		{
			BehaviourTreeNode* pNode = m_pNodes[i];

			if (pNode)
			{
				m_allocator.DeleteNode(pNode);
			}
		}

		m_current_nodes_count = 0;
		std::memset(m_pNodes, 0, sizeof(m_pNodes));
#ifdef _DEBUG
		m_debug_name[0] = '\0';
#endif
		m_allocator.Reset();
		m_shutdown_was_called = true;
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline const char* BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::GetName(void) const
	{
#ifdef _DEBUG
		return m_debug_name;
#else
		return nullptr;
#endif
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline void BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::GetAllNodes(BehaviourTreeNode**& pNodes, unsigned char& count)
	{
		pNodes = this->m_pNodes;
		count = this->m_nodes_count;
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline BehaviourTreeNode* BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::GetNode(unsigned char id) const
	{
		return m_pNodes[id];
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline unsigned char BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::GetMaxNodesCount(void) const
	{
		return MaxNodesInTree;
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline unsigned char BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::GetCurrentNodesCount(void) const
	{
		return m_current_nodes_count;
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline void BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::Dump(const char* pStackBuffer)
	{
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline void BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::PrintLogToConsole()
	{
		Assert(false && "todo: implement");
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline void BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::PrintLogToIDEConsole()
	{
		// todo: kirrik -> add sane preprocessors for determine platform type like SOLUNAR_PLATFORM_WINDOWS, SOLUNAR_PLATFORM_PS, SOLUNAR_PLATFORM_LINUX and etc
#ifdef _MSC_VER
		char buffer[256]{};

#endif
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	inline void BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::RebuildOrder()
	{
		BehaviourTreeNode* copy[MaxNodesInTree];

		constexpr size_t _SizeOf = sizeof(m_pNodes[0]) * MaxNodesInTree;
		std::memcpy(copy, m_pNodes,_SizeOf);
		
		unsigned char nodes_iter = 0;
		for (unsigned char i = 0; i < m_current_nodes_count; ++i)
		{
			if (copy[i])
			{
				// root node
				if (copy[i]->GetParentID() == kBehaviourTreeInvalidBaseType)
				{
					m_pNodes[nodes_iter] = copy[i];
					++nodes_iter;
				}
			}
		}

		for (unsigned char i = 0; i < m_current_nodes_count; ++i)
		{
			if (copy[i])
			{
				if (copy[i]->GetParentID() != kBehaviourTreeInvalidBaseType)
				{
					m_pNodes[nodes_iter] = copy[i];
					++nodes_iter;
				}
			}
		}
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	template<typename UserBehaviourTreeNodeType>
	inline UserBehaviourTreeNodeType* BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::AddNode(const char* pNodeDebugName)
	{
		size_t size_of_class = sizeof(UserBehaviourTreeNodeType);

		Assert(size_of_class != 0 && "can't be!");
		Assert(size_of_class != size_t(-1) && "something is wrong!");

		UserBehaviourTreeNodeType* pUserNode = nullptr;

		if (m_current_nodes_count == MaxNodesInTree)
			return pUserNode;

		void* pPlacementNewBufferPointer = m_allocator.AllocateNode(size_of_class);

		if (pPlacementNewBufferPointer)
		{
			pUserNode = new (pPlacementNewBufferPointer) UserBehaviourTreeNodeType(pNodeDebugName);
			m_pNodes[m_current_nodes_count] = pUserNode;
			pUserNode->SetID(m_current_nodes_count);
			++m_current_nodes_count;
		}

		return pUserNode;
	}

	template<typename Allocator, typename UserLogicDataType, unsigned char MaxNodesInTree>
	template<typename UserBehaviourTreeNodeType>
	inline UserBehaviourTreeNodeType* BehaviourTree<Allocator, UserLogicDataType, MaxNodesInTree>::AddNode(unsigned char parent_id, const char* pNodeDebugName)
	{
		BehaviourTreeNode* pParent = m_pNodes[parent_id];

		Assert(pParent && "can't find parent by id");
		UserBehaviourTreeNodeType* pChild = nullptr;
		if (pParent)
		{
			pChild = this->AddNode<UserBehaviourTreeNodeType>(pNodeDebugName);
			pParent->AddChild(pChild);
		}

		this->RebuildOrder();

		return pChild;
	}
}

#endif // !BEHAVIOUR_TREE_H
