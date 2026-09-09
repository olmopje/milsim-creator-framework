//! What happens when somebody shouts.
//!
//! THE SHOUT GOES TO A ROOM, NOT TO A PERSON. That is the whole reason this
//! exists instead of a list of orders on whoever you happen to be looking at:
//! you are not selecting a target, you are making a noise, and everyone within
//! earshot decides for themselves what to do about it. A crowd scattering
//! while one man stands his ground is the point.
//!
//! HONEST LIMIT: nothing in Arma Reforger lets a script raise a noise the AI
//! perception system can hear. Characters carry an EarsSensor with a decibel
//! threshold and the danger-reaction system handles gunfire, explosions and
//! vehicle horns -- but every one of those events is raised by the engine, and
//! no script in the entire game creates one. So this is a sphere around the
//! shouter, and a wall does not stop it unless MCF checks for one itself.
//!
//! EVERYTHING HERE RUNS ON THE SERVER. The client's part is a key and a noise.

enum MCF_EShout
{
	//! "Halt! Get down!"
	SURRENDER,
	//! "Stay back!"
	STAY_BACK
}

class MCF_AI_Shout
{
	//! How far a shout carries, in metres.
	protected static const float RANGE = 25;

	//! Collected by the sphere query, which cannot return a value.
	protected static ref array<IEntity> s_aHeard = {};

	//! Server side. Works out who heard, and lets each of them answer for
	//! themselves.
	//! \return How many people did what they were told.
	static int Resolve(notnull IEntity shouter, MCF_EShout shout, bool weaponRaised)
	{
		if (!Replication.IsServer())
			return 0;

		BaseWorld world = shouter.GetWorld();
		if (!world)
			return 0;

		s_aHeard.Clear();
		world.QueryEntitiesBySphere(shouter.GetOrigin(), RANGE, CollectEntity);

		int obeyed;

		foreach (IEntity heard : s_aHeard)
		{
			if (heard == shouter)
				continue;

			if (Answer(shouter, heard, shout, weaponRaised))
				obeyed++;
		}

		MCF_Core_Log.Debug("shout " + shout.ToString() + " heard by " + s_aHeard.Count().ToString() + ", obeyed by " + obeyed.ToString());
		return obeyed;
	}

	protected static bool CollectEntity(IEntity entity)
	{
		if (entity)
			s_aHeard.Insert(entity);

		return true;
	}

	//! One person's answer to one shout.
	protected static bool Answer(notnull IEntity shouter, notnull IEntity heard, MCF_EShout shout, bool weaponRaised)
	{
		MCF_AI_DispositionComponent disposition = MCF_AI_DispositionComponent.Cast(heard.FindComponent(MCF_AI_DispositionComponent));
		if (!disposition)
			return false;

		// Somebody already on the floor has nothing left to give up.
		if (disposition.GetCaptiveState() != MCF_ECaptiveState.FREE)
			return false;

		if (shout == MCF_EShout.STAY_BACK)
			return AnswerStayBack(shouter, heard, disposition);

		return AnswerSurrender(shouter, heard, disposition, weaponRaised);
	}

	// ------------------------------------------------------------ surrender

	protected static bool AnswerSurrender(notnull IEntity shouter, notnull IEntity heard, notnull MCF_AI_DispositionComponent disposition, bool weaponRaised)
	{
		MCF_AI_ComplianceComponent compliance = MCF_AI_ComplianceComponent.Cast(heard.FindComponent(MCF_AI_ComplianceComponent));
		if (!compliance)
			return false;

		float distance = vector.Distance(shouter.GetOrigin(), heard.GetOrigin());

		if (!compliance.WillSurrender(distance, weaponRaised, disposition.GetFear()))
		{
			// Shouting at an unarmed civilian for no reason costs you the
			// area, whether or not they do as they are told. The consequence
			// was written into the compliance component long before anything
			// called it.
			compliance.PunishIfUnjustified();
			return false;
		}

		compliance.PunishIfUnjustified();

		disposition.SetCaptiveState(MCF_ECaptiveState.COMPLIANT);
		disposition.AdjustFear(compliance.GetFearOnSurrender());

		DropWeapon(heard);
		HoldStill(heard);

		return true;
	}

	//! Puts whatever they are holding on the ground.
	//!
	//! This is the vanilla drop, the same one the player's own drop key ends
	//! at -- CharacterControllerComponent.DropWeapon, bracketed by the
	//! inventory lock, exactly as SCR_CharacterInventoryStorageComponent does
	//! it. Anything less and a surrendered man keeps his rifle in his hands.
	protected static void DropWeapon(notnull IEntity character)
	{
		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
			return;

		BaseWeaponManagerComponent weapons = controller.GetWeaponManagerComponent();
		if (!weapons)
			return;

		WeaponSlotComponent slot = weapons.GetCurrentSlot();
		if (!slot)
			return;

		SCR_InventoryStorageManagerComponent storage = SCR_InventoryStorageManagerComponent.Cast(controller.GetInventoryStorageManager());

		if (storage)
			storage.SetInventoryLocked(true);

		controller.DropWeapon(slot);

		if (storage)
			storage.SetInventoryLocked(false);
	}

	//! Stops them walking off. The behaviour tree is switched off outright
	//! rather than out-prioritised, because a surrendered man should not be
	//! deciding anything -- and it is reversed the moment somebody escorts
	//! them or lets them go.
	protected static void HoldStill(notnull IEntity character)
	{
		AIControlComponent control = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
		if (control)
			control.DeactivateAI();
	}

	// ------------------------------------------------------------ stay back

	protected static bool AnswerStayBack(notnull IEntity shouter, notnull IEntity heard, notnull MCF_AI_DispositionComponent disposition)
	{
		MCF_AI_ComplianceComponent compliance = MCF_AI_ComplianceComponent.Cast(heard.FindComponent(MCF_AI_ComplianceComponent));

		// Telling an armed man to stay back is not a thing. Tell him to drop
		// the weapon.
		if (compliance && compliance.IsArmed())
			return false;

		MCF_AI_SubjectControlComponent control = MCF_AI_SubjectControlComponent.Cast(heard.FindComponent(MCF_AI_SubjectControlComponent));
		if (!control)
			return false;

		control.Order(MCF_ESubjectOrder.STAND_OFF, shouter);

		// Being shouted at is frightening even when it is only about standing
		// somewhere else.
		disposition.AdjustFear(5);
		return true;
	}
}
