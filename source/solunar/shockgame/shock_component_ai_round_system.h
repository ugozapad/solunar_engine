// -----------------------------------------
//
//   author: wh1t3lord (https://github.com/wh1t3lord)
//	 description: Simple Round system for zombie waves
//
// -----------------------------------------

#ifndef SHOCK_AI_COMPONENT_ROUND_SYSTEM
#define SHOCK_AI_COMPONENT_ROUND_SYSTEM

namespace solunar
{
	const int kZombieSpawnerCount = 3;

	class ShockAIRoundSystem : public LogicComponent
	{
		DECLARE_OBJECT(ShockAIRoundSystem);
		DECLARE_PROPERTY_REGISTER(ShockAIRoundSystem);
	public:
		ShockAIRoundSystem();
		~ShockAIRoundSystem();

		void OnInit() override;

		void Update(float dt) override;

		void LoadXML(tinyxml2::XMLElement& element) override;
		void SaveXML(tinyxml2::XMLElement& element) override;

	private:
		bool IsPlayerKilledEnoughZombies();
		void NextRound();
		void FinishRound();
		void CompleteGame();
		void CompleteLevel();
			
		void SpawnZombie();

	private:
		Entity* m_spawnPoints[kZombieSpawnerCount];
		unsigned char m_current_round;
		unsigned char m_max_rounds;
		int m_zombies_to_kill;
		int m_numZombieSpawnedInCurrentRound;
		float m_timer;
	};

	extern ShockAIRoundSystem* g_ShockAIRoundSystem;
}

#endif