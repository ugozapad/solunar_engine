#ifndef RIGIDBODYCOMPONENT_H
#define RIGIDBODYCOMPONENT_H

#include "engine/physics/bullet_private.h"

namespace solunar {
	
	class PhysicsManager;
	class PhysicsWorld;
	class ShapeComponent;

	enum PhysicsFilter
	{
		PhysicsFilter_None = 1 << 6,
		PhysicsFilter_Player = 1 << 7,
		PhysicsFilter_Triggers = 1 << 8,
		PhysicsFilter_Usable = 1 << 9,
	};

	const int kPhysicsFilter_AllAux = PhysicsFilter_Player | PhysicsFilter_Triggers | PhysicsFilter_Usable;

	//! Rigid body component
	class RigidBodyComponent : public Component
	{
		DECLARE_OBJECT(RigidBodyComponent);
		DECLARE_PROPERTY_REGISTER(RigidBodyComponent);
	public:
		RigidBodyComponent();
		~RigidBodyComponent();
		
		static void RegisterObject();
		
		virtual void OnEntitySet(Entity* entity) override;
		virtual void OnWorldSet(World* world) override;

		virtual void OnEntityRemove() override;
		
		void LoadXML(tinyxml2::XMLElement& element) override;
		void SaveXML(tinyxml2::XMLElement& element) override;

		void UpdateEntityTranslationDirty();
		void UpdateBodyTranslationDirty();

		// Setter's and getter's
		const float GetMass() { return m_mass; } 
		void SetMass(float value) { m_mass = value; }

		const bool IsStatic() { return m_static; }

		btRigidBody* GetSDKBody() { return m_rigidBody; }
		btCompoundShape* GetCompoundShape() { return m_compoundShape; }
		PhysicsWorld* GetPhysicsWorld() { return m_physicsWorld; }

		//void getWorldTransform(btTransform& worldTrans) const override;
		//void setWorldTransform(const btTransform& worldTrans) override;

		void DisableCollide();
		void EnableCollide();

		void AttachShape(ShapeComponent* shape);
		void DettachShape(ShapeComponent* shape);

		void ApplyImpulse(const glm::vec3& impulse);

		void SetLinearVelocity(const glm::vec3& velocity);
		glm::vec3 GetLinearVelocity();

	private:
		void CreateBody();
		void DestroyBody();
		void UpdateBodyProperties();

	private:
		btRigidBody* m_rigidBody;
		btCompoundShape* m_compoundShape;

		PhysicsWorld* m_physicsWorld;

		// Body mass
		float m_mass;
		bool m_isEnable;
		bool m_static;
		bool m_isKinematic;
		bool m_isTrigger;
		bool m_inWorld;
	};

	//! Hack due support btCharacterController in the physics system.
	class RigidBodyProxyComponent : public RigidBodyComponent
	{
		DECLARE_OBJECT(RigidBodyProxyComponent);
	public:
		RigidBodyProxyComponent();
		~RigidBodyProxyComponent();

		static void RegisterObject();

		void OnEntitySet(Entity* entity) override;
		void OnWorldSet(World* world) override;

		void OnEntityRemove() override;

		void CreatePlayerController();
		void Update(float dt);
		void SetDirection(const glm::vec3& direction);
		void SetPositionForce(const glm::vec3& position);

		btKinematicCharacterController* GetCharacterController() { return m_characterController; }
		btGhostObject* GetGhostObject() { return m_ghostObject; }

	private:
		btKinematicCharacterController* m_characterController;
		btPairCachingGhostObject* m_ghostObject;
		btGhostPairCallback* m_ghostCallback;
		btConvexShape* m_capsuleShape;
	};
}

#endif // !RIGIDBODYCOMPONENT_H
