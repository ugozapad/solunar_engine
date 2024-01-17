#ifndef PHYSICSWORLD_H
#define PHYSICSWORLD_H

#include "physics/bullet_private.h"

namespace engine
{
	class RigidBodyComponent;

	class PhysicsWorld
	{
	public:
		PhysicsWorld();
		~PhysicsWorld();

		void addRigidBody(RigidBodyComponent* body);

		void step(float delta);

		btDynamicsWorld* getWorld() { return m_world; }

		float getFixedTimeStep() { return m_stepTime; }

		void toggleDebugDraw();

		void internalTick();

	private:
		btDefaultCollisionConfiguration* m_collisionConfiguration;
		btCollisionDispatcher* m_dispatcher;
		btBroadphaseInterface* m_overlappingPairCache;
		btSequentialImpulseConstraintSolver* m_solver;
		btDiscreteDynamicsWorld* m_world;
		btGhostPairCallback* m_btGhostPairCallback;

		float m_accumulatedTime;
		float m_stepTime;

		bool m_debugDraw;
	};
}

#endif // !PHYSICSWORLD_H
