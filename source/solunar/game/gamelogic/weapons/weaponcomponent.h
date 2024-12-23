#ifndef WEAPONCOMPONENT_H
#define WEAPONCOMPONENT_H

#include "engine/entity/logiccomponent.h"

namespace solunar {

	enum class WeaponsType
	{
		None,
		Pistol,
		Shotgun,
	};
	
	class WeaponComponent : public LogicComponent
	{
		DECLARE_OBJECT(WeaponComponent);
	public:
		WeaponComponent();
		virtual ~WeaponComponent();

		static void RegisterObject();

		virtual void Update(float dt);
		virtual void Update_Pistol(float dt);
		virtual void Update_Shotgun(float dt);

		int GetAmmo() { return m_ammo; }

		void SetWeaponType(WeaponsType type);

	private:
		bool m_inited;
		int m_idleAni;
		int m_fireAni;
		int m_prior_to_reload_Ani;
		int m_reload_one_Ani;
		int m_reload_last_one_Ani;

		int m_ammo;
		int m_clipSize;

		WeaponsType m_type;
	};

	// Global flag to enable the trace debugging from weapons,
	//  if ENABLE_TRACE_DEBUG is enabled
	extern bool g_debugTrace;

}
#endif // ! WEAPONCOMPONENT_H