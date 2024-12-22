// -----------------------------------------
//
//   author: wh1t3lord (https://github.com/wh1t3lord)
//	 description: Frontend component for storing backend of behaviour tree implementation (see behaviour tree factory impl)
//
// -----------------------------------------

#ifndef SHOCK_AI_BEHAVIOURTREE
#define SHOCK_AI_BEHAVIOURTREE

namespace solunar
{
	class IBehaviourTree;
	enum eShockBehaviourTree;
}

namespace solunar 
{
	class ShockAIBehaviourTree : public LogicComponent
	{
		DECLARE_OBJECT(ShockAIBehaviourTree);
		DECLARE_PROPERTY_REGISTER(ShockAIBehaviourTree);
	public:
		ShockAIBehaviourTree();
		~ShockAIBehaviourTree();

		void Update(float dt) override;

		void LoadXML(tinyxml2::XMLElement& element) override;
		void SaveXML(tinyxml2::XMLElement& element) override;

	private:
		eShockBehaviourTree m_tree_type;
		IBehaviourTree* m_pTree;
	};
}

#endif