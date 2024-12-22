#include "enginepch.h"
#include "engine/physics/rigidbodycomponent.h"
#include "engine/physics/physicsworld.h"
#include "engine/physics/shapescomponent.h"

#include "engine/entity/world.h"

#include "core/timer.h"

namespace solunar {

	IMPLEMENT_OBJECT(RigidBodyComponent, Component);

	BEGIN_PROPERTY_REGISTER(RigidBodyComponent)
	{
		REGISTER_PROPERTY(RigidBodyComponent, PropertyFloat, m_mass);
		REGISTER_PROPERTY(RigidBodyComponent, PropertyBool, m_isEnable);
		REGISTER_PROPERTY(RigidBodyComponent, PropertyBool, m_static);
		REGISTER_PROPERTY(RigidBodyComponent, PropertyBool, m_isKinematic);
		REGISTER_PROPERTY(RigidBodyComponent, PropertyBool, m_isTrigger);
	//	RegisterProperty(RigidBodyComponent, PropertyBool, m_inWorld); INTERNAL !!!
	}
	END_PROPERTY_REGISTER(RigidBodyComponent)

	RigidBodyComponent::RigidBodyComponent()
	{
		m_rigidBody = nullptr;
		m_compoundShape = nullptr;
		m_physicsWorld = nullptr;

		m_mass = 0.0f;
		m_isEnable = false;
		m_static = false;
		m_isKinematic = false;
		m_isTrigger = false;
		m_inWorld = false;
	}

	RigidBodyComponent::~RigidBodyComponent()
	{
		m_physicsWorld = nullptr;
		m_compoundShape = nullptr;
		m_rigidBody = nullptr;
	}

	void RigidBodyComponent::RegisterObject()
	{
		g_typeManager->RegisterObject<RigidBodyComponent>();
	}

	void RigidBodyComponent::OnEntitySet(Entity* entity)
	{
		Component::OnEntitySet(entity);

		// #TODO: WEIRD STUFF !!!
		//if (m_physicsWorld && !m_rigidBody)
		//	CreateBody();
	}

	void RigidBodyComponent::OnWorldSet(World* world)
	{
		Component::OnWorldSet(world);
		m_physicsWorld = world->GetPhysicsWorld();
	}

	void RigidBodyComponent::OnEntityRemove()
	{
		DestroyBody();
		Component::OnEntityRemove();
	}

	void RigidBodyComponent::LoadXML(tinyxml2::XMLElement& element)
	{
		Component::LoadXML(element);

		tinyxml2::XMLElement* massElement = element.FirstChildElement("Mass");
		if (massElement)
			massElement->QueryFloatAttribute("value", &m_mass);

		tinyxml2::XMLElement* staticElement = element.FirstChildElement("Static");
		if (staticElement)
			staticElement->QueryBoolAttribute("value", &m_static);

		tinyxml2::XMLElement* triggerElement = element.FirstChildElement("Trigger");
		if (triggerElement)
			triggerElement->QueryBoolAttribute("value", &m_isTrigger);

		bool changeFilterUsableHack = false;

		tinyxml2::XMLElement* filterElement = element.FirstChildElement("Filter");
		if (filterElement)
		{
			const char* filterValue = nullptr;
			filterElement->QueryStringAttribute("value", &filterValue);
			if (filterValue)
			{
				if (strcmp(filterValue, "PhysicsFilter_Usable") == 0)
				{
					changeFilterUsableHack = true;
				}
			}
		}

		CreateBody();
		UpdateBodyTranslationDirty();

		if (changeFilterUsableHack)
		{
			m_rigidBody->getBroadphaseProxy()->m_collisionFilterGroup = PhysicsFilter_Usable;
		}
		else if (m_isTrigger)
		{
			m_rigidBody->getBroadphaseProxy()->m_collisionFilterGroup = PhysicsFilter_Triggers;
		}
	}

	void RigidBodyComponent::SaveXML(tinyxml2::XMLElement& element)
	{
		Component::SaveXML(element);

		tinyxml2::XMLElement* enableElement = element.InsertNewChildElement("Enable");
		enableElement->SetAttribute("value", m_isEnable);

		tinyxml2::XMLElement* massElement = element.InsertNewChildElement("Mass");
		massElement->SetAttribute("value", m_mass);

		tinyxml2::XMLElement* staticElement = element.InsertNewChildElement("Static");
		staticElement->SetAttribute("value", m_static);

		tinyxml2::XMLElement* kinematicElement = element.InsertNewChildElement("Kinematic");
		kinematicElement->SetAttribute("value", m_isKinematic);

		tinyxml2::XMLElement* triggerElement = element.InsertNewChildElement("Trigger");
		triggerElement->SetAttribute("value", m_isTrigger);
	}

	void RigidBodyComponent::UpdateEntityTranslationDirty()
	{
		btTransform trans = m_rigidBody->getWorldTransform();
		GetEntity()->SetPosition(btVectorToGlm(trans.getOrigin()));

		// calculate rotation
		btQuaternion quaternion;
		trans.getBasis().getRotation(quaternion);
		GetEntity()->SetRotation(glm::quat(quaternion.getW(), quaternion.getX(), quaternion.getY(), quaternion.getZ()));

		// calculate rotation based on euler angles
		//float roll, pitch, yaw;
		//trans.getRotation().getEulerZYX(yaw, pitch, roll);
		//GetEntity()->SetEulerRotation(glm::vec3(roll, pitch, yaw));
	}

	void RigidBodyComponent::UpdateBodyTranslationDirty()
	{
		if (!m_rigidBody)
			return;

		btTransform trans;
		trans.setIdentity();

		// origin
		trans.setOrigin(glmVectorToBt(GetEntity()->GetPosition()));

		// rotation
		glm::quat entityRot = GetEntity()->GetRotation();
		btQuaternion bodyRot;
		bodyRot.setX(entityRot.x);
		bodyRot.setY(entityRot.y);
		bodyRot.setZ(entityRot.z);
		bodyRot.setW(entityRot.w);
		trans.setRotation(bodyRot);

		// set transformation to body
		m_rigidBody->setWorldTransform(trans);
	}

#if 0
	void RigidBodyComponent::getWorldTransform(btTransform& worldTrans) const
	{
		//throw std::logic_error("The method or operation is not implemented.");
	}

	void RigidBodyComponent::setWorldTransform(const btTransform& worldTrans)
	{
		//throw std::logic_error("The method or operation is not implemented.");

		UpdateBodyTranslationDirty();
	}
#endif

	void RigidBodyComponent::CreateBody()
	{
		Assert(m_physicsWorld);

		if (m_rigidBody)
			return;

		m_compoundShape = mem_new<btCompoundShape>();

		btVector3 localInertia(0.0f, 0.0f, 0.0f);
		btRigidBody::btRigidBodyConstructionInfo rigidBodyCInfo(m_mass, nullptr, m_compoundShape, localInertia);
		m_rigidBody = mem_new<btRigidBody>(rigidBodyCInfo);
		m_rigidBody->setUserPointer(this);

		UpdateBodyProperties();

		m_physicsWorld->AddRigidBody(this);
		m_inWorld = m_rigidBody->isInWorld();
	}

	void RigidBodyComponent::DestroyBody()
	{
		if (m_physicsWorld)
		{
			m_physicsWorld->RemoveRigidBody(this);
		}
		else
		{
			Core::Msg("RigidBodyComponent::DestroyBody: Can't remove body. Physics world is already destroyed.");
			Core::Msg("Entity=0x%p", GetEntity());
		}
		
		// remove user pointer from this component
		m_rigidBody->setUserPointer(nullptr);

		mem_delete(m_rigidBody);
		m_rigidBody = nullptr;

		mem_delete(m_compoundShape);
		m_compoundShape = nullptr;
	}

	void RigidBodyComponent::UpdateBodyProperties()
	{
		int bodyCollisionFlags = m_rigidBody->getCollisionFlags();

		if (m_static)
			bodyCollisionFlags |= btRigidBody::CF_STATIC_OBJECT;
		else
			bodyCollisionFlags &= ~btRigidBody::CF_STATIC_OBJECT;

		if (m_isKinematic)
			bodyCollisionFlags |= btRigidBody::CF_KINEMATIC_OBJECT;
		else
			bodyCollisionFlags &= ~btRigidBody::CF_KINEMATIC_OBJECT;

		if (m_isTrigger)
			bodyCollisionFlags |= btRigidBody::CF_NO_CONTACT_RESPONSE;
		else
			bodyCollisionFlags &= ~btRigidBody::CF_NO_CONTACT_RESPONSE;

		m_rigidBody->setCollisionFlags(bodyCollisionFlags);

		// trigger custom color
		if (m_isTrigger)
			m_rigidBody->setCustomDebugColor(btVector3(1.0f, 0.5f, 0.0f));
	}

	void RigidBodyComponent::DisableCollide()
	{
		m_rigidBody->setCollisionFlags(m_rigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}

	void RigidBodyComponent::EnableCollide()
	{
		m_rigidBody->setCollisionFlags(m_rigidBody->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
	}

	void RigidBodyComponent::AttachShape(ShapeComponent* shape)
	{
		shape->InitializeShape();
	}

	void RigidBodyComponent::DettachShape(ShapeComponent* shape)
	{
		m_compoundShape->removeChildShape(shape->getSdkShape());
	}

	void RigidBodyComponent::ApplyImpulse(const glm::vec3& impulse)
	{
		m_rigidBody->applyCentralImpulse(glmVectorToBt(impulse));
	}

	void RigidBodyComponent::SetLinearVelocity(const glm::vec3& velocity)
	{
		m_rigidBody->setLinearVelocity(glmVectorToBt(velocity));
	}

	glm::vec3 RigidBodyComponent::GetLinearVelocity()
	{
		return btVectorToGlm(m_rigidBody->getLinearVelocity());
	}

	/////////////////////////////////////////////////////////////////

	IMPLEMENT_OBJECT(RigidBodyProxyComponent, RigidBodyComponent);

	RigidBodyProxyComponent::RigidBodyProxyComponent() :
		m_characterController(nullptr),
		m_ghostObject(nullptr),
		m_ghostCallback(nullptr),
		m_capsuleShape(nullptr)
	{
	}

	RigidBodyProxyComponent::~RigidBodyProxyComponent()
	{
		GetPhysicsWorld()->GetWorld()->removeAction(m_characterController);
		GetPhysicsWorld()->GetWorld()->removeCollisionObject(m_ghostObject);
		m_ghostObject->setCollisionShape(nullptr);
		GetPhysicsWorld()->GetWorld()->getPairCache()->setInternalGhostPairCallback(nullptr);

		m_ghostObject->setUserPointer(nullptr);

		mem_delete(m_capsuleShape);
		mem_delete(m_ghostCallback);
		mem_delete(m_ghostObject);
		mem_delete(m_characterController);
	}

	void RigidBodyProxyComponent::RegisterObject()
	{
		g_typeManager->RegisterObject<RigidBodyProxyComponent>();
	}

	void RigidBodyProxyComponent::OnEntitySet(Entity* entity)
	{
		Component::OnEntitySet(entity);
	}

	void RigidBodyProxyComponent::OnWorldSet(World* world)
	{
		RigidBodyComponent::OnWorldSet(world);
	}

	void RigidBodyProxyComponent::OnEntityRemove()
	{
		Component::OnEntityRemove();
	}

	void RigidBodyProxyComponent::CreatePlayerController()
	{
		btTransform startTransform; startTransform.setIdentity();
		startTransform.setOrigin(glmVectorToBt(GetEntity()->GetPosition()));

		float fRadius = 0.3f;
		float fHeight = 1.0f;
		m_capsuleShape = mem_new<btCapsuleShapeZ>(fRadius, fHeight);

		m_ghostObject = mem_new<btPairCachingGhostObject>();
		m_ghostObject->setWorldTransform(startTransform);
		m_ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
		m_ghostObject->setUserPointer(this);

		m_ghostCallback = mem_new<btGhostPairCallback>();
		GetPhysicsWorld()->GetWorld()->getPairCache()->setInternalGhostPairCallback(m_ghostCallback);

		m_ghostObject->setCollisionShape(m_capsuleShape);

		m_characterController = mem_new<btKinematicCharacterController>(
			m_ghostObject, 
			m_capsuleShape, 
			0.25f, 
			btVector3(0.0f, 1.0f, 0.0f));

		m_characterController->setMaxJumpHeight(0.8f);
		m_characterController->setGravity(GetPhysicsWorld()->GetWorld()->getGravity());

		GetPhysicsWorld()->GetWorld()->addCollisionObject(
			m_ghostObject,
			btBroadphaseProxy::CharacterFilter,
			kCollisionFilterAllMask | PhysicsFilter_Player
		);

		GetPhysicsWorld()->GetWorld()->addAction(m_characterController);
	}

	void RigidBodyProxyComponent::Update(float dt)
	{
		m_characterController->updateAction(GetPhysicsWorld()->GetWorld(), dt);
	}

	void RigidBodyProxyComponent::SetDirection(const glm::vec3& direction)
	{
		m_characterController->setWalkDirection(glmVectorToBt(direction));
	}

	void RigidBodyProxyComponent::SetPositionForce(const glm::vec3& position)
	{
		btTransform trans;
		trans.setIdentity();
		trans.setOrigin(glmVectorToBt(position));
		m_ghostObject->setWorldTransform(trans);
	}
}