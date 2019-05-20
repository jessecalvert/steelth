/*@H
* File: steelth_ability.cpp
* Author: Jesse Calvert
* Created: November 6, 2018, 16:24
* Last modified: April 10, 2019, 18:43
*/

inline s32
HexDistance(v2i A, v2i B)
{
	s32 Az = -A.x - A.y;
	s32 Bz = -B.x - B.y;
	s32 Result = (s32)(AbsoluteValue((f32)(B.x - A.x)) +
					   AbsoluteValue((f32)(B.y - A.y)) +
					   AbsoluteValue((f32)(Bz - Az)))/2;
	return Result;
}

internal s32
EntityHexDistance(entity *A, entity *B)
{
	s32 Result = S32Maximum;
	if((A->Flags & EntityFlag_OnBoard) && (B->Flags & EntityFlag_OnBoard))
	{
		Result = HexDistance(A->BoardPosition, B->BoardPosition);
	}

	return Result;
}

internal entity *
GetOpponent(game_mode_world *WorldMode, entity *Player)
{
	Assert((Player->Owner == Player_1) || (Player->Owner == Player_2));
	player_name OpponentName = (Player->Owner == Player_1) ? Player_2 : Player_1;
	entity *Opponent = GetEntity(WorldMode->EntityManager, WorldMode->Players[OpponentName]);
	return Opponent;
}

inline board_tile *
GetTile(board *Board, v2i BoardPosition)
{
	board_tile *Tile = Board->Tiles + (Board->Width * BoardPosition.y) + BoardPosition.x;
	Assert(Tile->BoardPosition == BoardPosition);
	return Tile;
}

internal entity_list
GetEntitiesAtPosition(board *Board, v2i BoardPosition)
{
	board_tile *Tile = GetTile(Board, BoardPosition);
	entity_list Result = Tile->List;
	return Result;
}

internal b32
TileIsTraversable(game_mode_world *WorldMode, v2i BoardPosition)
{
	b32 Result = true;
	entity_list List = GetEntitiesAtPosition(&WorldMode->Board, BoardPosition);
	for(u32 EntityIndex = 0;
		EntityIndex < List.EntityCount;
		++EntityIndex)
	{
		entity_id ID = List.EntityIDs[EntityIndex];
		entity *Entity = GetEntity(WorldMode->EntityManager, ID);
		if(Entity && !(Entity->Flags & EntityFlag_Traversable))
		{
			Result = false;
			break;
		}
	}

	return Result;
}

internal ability *
AddAbility(game_mode_world *WorldMode, entity *Entity, ability_name AbilityName)
{
	Assert(Entity->AbilityCount < ArrayCount(Entity->Abilities));
	ability *Ability = WorldMode->Abilities + AbilityName;
	u32 AbilityIndex = Entity->AbilityCount++;
	ability *Result = Entity->Abilities + AbilityIndex;
	*Result = *Ability;
	Entity->Abilities[AbilityIndex].CurrentCooldown = Entity->Abilities[AbilityIndex].Cooldown;
	return Result;
}

internal void
PlaceEntityOnBoard(game_mode_world *WorldMode, entity *Entity, v2i BoardPosition)
{
	board_tile *Tile = GetTile(&WorldMode->Board, BoardPosition);
	v3 P = Tile->P;
	v3 Dim = Tile->Dim;

	Entity->P = P;
	Entity->Rotation = Q(1,0,0,0);

	Entity->Flags |= EntityFlag_OnBoard;
	Entity->BoardPosition = BoardPosition;
}

internal entity *
CreateEntityOnBoard(game_mode_world *WorldMode, entity_manager *EntityManager, assets *Assets,
	entity_type Type, v2i BoardPosition, player_name Owner = Player_None)
{
	entity *Result = CreateEntity(WorldMode, WorldMode->EntityManager, Assets, Type);
	PlaceEntityOnBoard(WorldMode, Result, BoardPosition);
	Result->Owner = Owner;
	return Result;
}

internal void
CreateBoardEntityRigidBody(game_state *GameState, game_mode_world *WorldMode, entity *Entity)
{
	v3 Dim = WorldMode->GroundDim;
	collider Sentinel = {};
	DLIST_INIT(&Sentinel);
	collider *Collider0 = BuildBoxCollider(&GameState->FrameRegion, &WorldMode->Physics,
		V3(0,0,0), Dim, NoRotation());
	DLIST_INSERT(&Sentinel, Collider0);

	move_spec MoveSpec = DefaultMoveSpec();
	InitializeRigidBody(&WorldMode->Physics, Entity, &Sentinel, true, MoveSpec);
}

internal entity *
AddGround(game_state *GameState, game_mode_world *WorldMode, v2i BoardPosition)
{
	entity *Result = CreateEntityOnBoard(WorldMode, WorldMode->EntityManager, GameState->Assets, Entity_Ground, BoardPosition);
	CreateBoardEntityRigidBody(GameState, WorldMode, Result);
	return Result;
}

internal entity *
AddWater(game_state *GameState, game_mode_world *WorldMode, v2i BoardPosition)
{
	entity *Result = CreateEntityOnBoard(WorldMode, WorldMode->EntityManager, GameState->Assets, Entity_Water, BoardPosition);
	CreateBoardEntityRigidBody(GameState, WorldMode, Result);
	return Result;
}

internal entity *
AddCard(game_state *GameState, game_mode_world *WorldMode, player_name Owner, ability_name AbilityName)
{
	entity *Result = CreateEntity(WorldMode, WorldMode->EntityManager, GameState->Assets, Entity_Card);
	Result->Owner = Owner;

	Result->BaseAnimation.Type = Animation_Spring;
	spring_animation *SpringAnim = &Result->BaseAnimation.SpringAnim;
	SpringAnim->OscillationFrequency = 8.0f;
	SpringAnim->OscillationReductionPerSecond = 5.0f;

	v3 Dim = WorldMode->CardDim;
	collider Sentinel = {};
	DLIST_INIT(&Sentinel);
	collider *Collider0 = BuildBoxCollider(&GameState->FrameRegion, &WorldMode->Physics,
		V3(0,0,0), Dim, NoRotation());
	DLIST_INSERT(&Sentinel, Collider0);
	move_spec MoveSpec = DefaultMoveSpec();
	InitializeRigidBody(&WorldMode->Physics, Result, &Sentinel, true, MoveSpec);

	ability *Ability = AddAbility(WorldMode, Result, AbilityName);

	return Result;
}

internal void
InitAbilities(memory_region *Region, ability *Abilities)
{
	ability *Move = Abilities + Ability_Move;
	Move->Name = Ability_Move;
	Move->Type = AbilityType_Activated;
	Move->DisplayName = ConstZ("Move");
	Move->DescriptionFormat = "Move unit to empty board position.";
	Move->Cooldown = 1;
	Move->TargetFlags = Target_OnBoard|Target_InRange|Target_Traversable;
	Move->TargetCount = 1;
	Move->Range = 1;

	ability *Attack = Abilities + Ability_Attack;
	Attack->Name = Ability_Attack;
	Attack->Type = AbilityType_Activated;
	Attack->DisplayName = ConstZ("Attack");
	Attack->DescriptionFormat = "Attack a unit.";
	Attack->Cooldown = 1;
	Attack->TargetFlags = Target_OnBoard|Target_InRange;
	Attack->TargetCount = 1;
	Attack->Range = 1;

	ability *GenerateCharge = Abilities + Ability_GenerateCharge;
	GenerateCharge->Name = Ability_GenerateCharge;
	GenerateCharge->Type = AbilityType_Reaction;
	GenerateCharge->DisplayName = ConstZ("Generate Charge");
	GenerateCharge->DescriptionFormat = "Gain %d charges.";
	GenerateCharge->Parameters[0] = 1;
	GenerateCharge->Trigger = Reaction_TurnStart;

	{
		ability *SummonWarrior = Abilities + Ability_SummonWarrior;
		SummonWarrior->Name = Ability_SummonWarrior;
		SummonWarrior->Type = AbilityType_Activated;
		SummonWarrior->DisplayName = ConstZ("Summon Warrior");
		SummonWarrior->DescriptionFormat = "Summon a Warrior with Fireball.";
		SummonWarrior->TargetFlags = Target_OnBoard|Target_Traversable|Target_Influenced;
		SummonWarrior->TargetCount = 1;
		SummonWarrior->Range = S32Maximum;
		SummonWarrior->Cost.Gold = 2;
		SummonWarrior->Cost.Actions = 1;

		ability *Fireball = Abilities + Ability_Fireball;
		Fireball->Name = Ability_Fireball;
		Fireball->Type = AbilityType_Activated;
		Fireball->DisplayName = ConstZ("Fireball");
		Fireball->DescriptionFormat = "Deal %d damage to a target.";
		Fireball->TargetCount = 1;
		Fireball->TargetFlags = Target_OnBoard|Target_InRange;
		Fireball->Range = 2;
		Fireball->Parameters[0] = 2;
		Fireball->Cooldown = 2;
	}

	{
		ability *SummonMercenaries = Abilities + Ability_SummonMercenaries;
		SummonMercenaries->Name = Ability_SummonMercenaries;
		SummonMercenaries->Type = AbilityType_Activated;
		SummonMercenaries->DisplayName = ConstZ("Summon Mercenaries");
		SummonMercenaries->DescriptionFormat = "Summon Mercenaries with a cool ability.";
		SummonMercenaries->TargetFlags = Target_OnBoard|Target_Traversable|Target_Influenced;
		SummonMercenaries->TargetCount = 1;
		SummonMercenaries->Range = S32Maximum;
		SummonMercenaries->Cost.Gold = 5;
		SummonMercenaries->Cost.Actions = 1;

		ability *HireMercenaries = Abilities + Ability_HireMercenaries;
		HireMercenaries->Name = Ability_HireMercenaries;
		HireMercenaries->Type = AbilityType_Activated;
		HireMercenaries->DisplayName = ConstZ("Hire Mercenaries");
		HireMercenaries->DescriptionFormat = "Give unit %d attack.";
		HireMercenaries->Parameters[0] = 2;
		HireMercenaries->Cost.Gold = 4;
		HireMercenaries->Cost.Actions = 1;
	}

	{
		ability *BuildBank = Abilities + Ability_BuildBank;
		BuildBank->Name = Ability_BuildBank;
		BuildBank->Type = AbilityType_Activated;
		BuildBank->DisplayName = ConstZ("Build Bank");
		BuildBank->DescriptionFormat = "Lose all gold. Build a bank with that many charges.";
		BuildBank->TargetFlags = Target_OnBoard|Target_Traversable|Target_Influenced;
		BuildBank->TargetCount = 1;
		BuildBank->Range = S32Maximum;
		BuildBank->Cost.Actions = 1;

		ability *BankActive = Abilities + Ability_BankActive;
		BankActive->Name = Ability_BankActive;
		BankActive->Type = AbilityType_Activated;
		BankActive->DisplayName = ConstZ("Withdraw");
		BankActive->DescriptionFormat = "Gain %d gold.";
		BankActive->Parameters[0] = 2;
		BankActive->Cost.Charges = 1;
		BankActive->Cost.Actions = 1;

		ability *BankPillaged = Abilities + Ability_BankPillaged;
		BankPillaged->Name = Ability_BankPillaged;
		BankPillaged->Type = AbilityType_Reaction;
		BankPillaged->DisplayName = ConstZ("Bank Pillaged");
		BankPillaged->DescriptionFormat = "Lose a charge. If you do, opponent gains %d gold.";
		BankPillaged->Parameters[0] = 2;
		BankPillaged->Trigger = Reaction_OnDestroy;
	}

	{
		ability *BuildRiskInvestment = Abilities + Ability_BuildRiskInvestment;
		BuildRiskInvestment->Name = Ability_BuildRiskInvestment;
		BuildRiskInvestment->Type = AbilityType_Activated;
		BuildRiskInvestment->DisplayName = ConstZ("Build Risk Investment");
		BuildRiskInvestment->DescriptionFormat = "Gain gold when this gets to 2? charges.";
		BuildRiskInvestment->TargetFlags = Target_OnBoard|Target_Traversable|Target_Influenced;
		BuildRiskInvestment->TargetCount = 1;
		BuildRiskInvestment->Range = S32Maximum;
		BuildRiskInvestment->Cost.Gold = 3;
		BuildRiskInvestment->Cost.Actions = 1;

		ability *RiskInvestmentReturn = Abilities + Ability_RiskInvestmentReturn;
		RiskInvestmentReturn->Name = Ability_RiskInvestmentReturn;
		RiskInvestmentReturn->Type = AbilityType_Reaction;
		RiskInvestmentReturn->DisplayName = ConstZ("Investment Return");
		RiskInvestmentReturn->DescriptionFormat = "When this has %d charges, destroy it and gain %d gold.";
		RiskInvestmentReturn->Parameters[0] = 2;
		RiskInvestmentReturn->Parameters[1] = 7;
		RiskInvestmentReturn->Trigger = Reaction_OnChargeChange;

		ability *RiskInvestmentPillaged = Abilities + Ability_RiskInvestmentPillaged;
		RiskInvestmentPillaged->Name = Ability_RiskInvestmentPillaged;
		RiskInvestmentPillaged->Type = AbilityType_Reaction;
		RiskInvestmentPillaged->DisplayName = ConstZ("Risk Investment Pillaged");
		RiskInvestmentPillaged->DescriptionFormat = "Destroy this building.";
		RiskInvestmentPillaged->Trigger = Reaction_OnDestroy;
	}

	{
		ability *BuildGoldMine = Abilities + Ability_BuildGoldMine;
		BuildGoldMine->Name = Ability_BuildGoldMine;
		BuildGoldMine->Type = AbilityType_Activated;
		BuildGoldMine->DisplayName = ConstZ("Build Gold Mine");
		BuildGoldMine->DescriptionFormat = "Gains gold every turn.";
		BuildGoldMine->TargetFlags = Target_OnBoard|Target_Traversable|Target_Influenced;
		BuildGoldMine->TargetCount = 1;
		BuildGoldMine->Range = S32Maximum;
		BuildGoldMine->Cost.Gold = 5;
		BuildGoldMine->Cost.Actions = 1;

		ability *GoldMine = Abilities + Ability_GoldMine;
		GoldMine->Name = Ability_GoldMine;
		GoldMine->Type = AbilityType_Reaction;
		GoldMine->DisplayName = ConstZ("Mine Gold");
		GoldMine->DescriptionFormat = "Gain %d gold.";
		GoldMine->Trigger = Reaction_TurnStart;
		GoldMine->Parameters[0] = 2;

		ability *GoldMinePillaged = Abilities + Ability_GoldMinePillaged;
		GoldMinePillaged->Name = Ability_GoldMinePillaged;
		GoldMinePillaged->Type = AbilityType_Reaction;
		GoldMinePillaged->DisplayName = ConstZ("Gold Mine Pillaged");
		GoldMinePillaged->DescriptionFormat = "Lose %d gold.";
		GoldMinePillaged->Trigger = Reaction_OnDestroy;
		GoldMinePillaged->Parameters[0] = 2;
	}

	{
		ability *BuildPowerPlant = Abilities + Ability_BuildPowerPlant;
		BuildPowerPlant->Name = Ability_BuildPowerPlant;
		BuildPowerPlant->Type = AbilityType_Activated;
		BuildPowerPlant->DisplayName = ConstZ("Build Power Plant");
		BuildPowerPlant->DescriptionFormat = "Gives charges to nearby tiles.";
		BuildPowerPlant->TargetFlags = Target_OnBoard|Target_Traversable|Target_Influenced;
		BuildPowerPlant->TargetCount = 1;
		BuildPowerPlant->Range = S32Maximum;
		BuildPowerPlant->Cost.Gold = 4;
		BuildPowerPlant->Cost.Actions = 1;

		ability *GeneratePower = Abilities + Ability_GeneratePower;
		GeneratePower->Name = Ability_GeneratePower;
		GeneratePower->Type = AbilityType_Reaction;
		GeneratePower->DisplayName = ConstZ("Generate Power");
		GeneratePower->DescriptionFormat = "Anything %d tiles away gains %d charges.";
		GeneratePower->Parameters[0] = 1;
		GeneratePower->Parameters[1] = 1;
		GeneratePower->Trigger = Reaction_TurnStart;

		ability *PowerPlantPillaged = Abilities + Ability_PowerPlantPillaged;
		PowerPlantPillaged->Name = Ability_PowerPlantPillaged;
		PowerPlantPillaged->Type = AbilityType_Reaction;
		PowerPlantPillaged->DisplayName = ConstZ("Power Plant Pillaged");
		PowerPlantPillaged->DescriptionFormat = "Opponent takes control of Power Plant.";
		PowerPlantPillaged->Trigger = Reaction_OnDestroy;
	}

	{
		ability *BuildHeadquarters = Abilities + Ability_BuildHeadquarters;
		BuildHeadquarters->Name = Ability_BuildHeadquarters;
		BuildHeadquarters->Type = AbilityType_Activated;
		BuildHeadquarters->DisplayName = ConstZ("Build HQ");
		BuildHeadquarters->DescriptionFormat = "Build your HQ. It's pretty important.";
		BuildHeadquarters->TargetFlags = Target_OnBoard|Target_Traversable;
		BuildHeadquarters->TargetCount = 1;
		BuildHeadquarters->Range = S32Maximum;
		BuildHeadquarters->Cost.Actions = 1;

		ability *HQPillaged = Abilities + Ability_HQPillaged;
		HQPillaged->Name = Ability_HQPillaged;
		HQPillaged->Type = AbilityType_Reaction;
		HQPillaged->DisplayName = ConstZ("HQ Pillaged");
		HQPillaged->DescriptionFormat = "Opponent gains %d victory points.";
		HQPillaged->Parameters[0] = 1;
		HQPillaged->Trigger = Reaction_OnDestroy;

		ability *DrawCard = Abilities + Ability_DrawCard;
		DrawCard->Name = Ability_DrawCard;
		DrawCard->Type = AbilityType_Activated;
		DrawCard->DisplayName = ConstZ("Draw a Card");
		DrawCard->DescriptionFormat = "Draws a card.";
		DrawCard->Parameters[0] = 1;
		DrawCard->Cost.Actions = 1;

		ability *GainGold = Abilities + Ability_GainGold;
		GainGold->Name = Ability_GainGold;
		GainGold->Type = AbilityType_Activated;
		GainGold->DisplayName = ConstZ("Gain Gold");
		GainGold->DescriptionFormat = "Gain %d gold.";
		GainGold->Parameters[0] = 1;
		GainGold->Cost.Actions = 1;

		ability *GatherResources = Abilities + Ability_GatherResources;
		GatherResources->Name = Ability_GatherResources;
		GatherResources->Type = AbilityType_Reaction;
		GatherResources->DisplayName = ConstZ("Gather Resources");
		GatherResources->DescriptionFormat = "Gather resources from all tiles %d away.";
		GatherResources->Parameters[0] = 1;
		GatherResources->Trigger = Reaction_TurnStart;
	}

	ability *HedgeFund = Abilities + Ability_HedgeFund;
	HedgeFund->Name = Ability_HedgeFund;
	HedgeFund->Type = AbilityType_Activated;
	HedgeFund->DisplayName = ConstZ("Hedge Fund");
	HedgeFund->DescriptionFormat = "Gain %d gold.";
	HedgeFund->Range = S32Maximum;
	HedgeFund->Parameters[0] = 9;
	HedgeFund->Cost.Gold = 4;
	HedgeFund->Cost.Actions = 1;

	ability *Buyout = Abilities + Ability_Buyout;
	Buyout->Name = Ability_Buyout;
	Buyout->Type = AbilityType_Activated;
	Buyout->DisplayName = ConstZ("Buyout");
	Buyout->DescriptionFormat = "Take control of a building.";
	Buyout->TargetCount = 1;
	Buyout->TargetFlags = Target_OnBoard|Target_Building|Target_Influenced;
	Buyout->Range = S32Maximum;
	Buyout->Cost.Gold = 15;
	Buyout->Cost.Actions = 1;

	ability *Bribe = Abilities + Ability_Bribe;
	Bribe->Name = Ability_Bribe;
	Bribe->Type = AbilityType_Activated;
	Bribe->DisplayName = ConstZ("Bribe");
	Bribe->DescriptionFormat = "Take control of a unit.";
	Bribe->TargetCount = 1;
	Bribe->TargetFlags = Target_OnBoard|Target_Unit|Target_Influenced;
	Bribe->Range = S32Maximum;
	Bribe->Cost.Gold = 12;
	Bribe->Cost.Actions = 1;

	ability *Fortify = Abilities + Ability_Fortify;
	Fortify->Name = Ability_Fortify;
	Fortify->Type = AbilityType_Activated;
	Fortify->DisplayName = ConstZ("Fortify");
	Fortify->DescriptionFormat = "Target gains %d max defense.";
	Fortify->TargetCount = 1;
	Fortify->TargetFlags = Target_OnBoard|Target_Influenced;
	Fortify->Range = S32Maximum;
	Fortify->Parameters[0] = 3;
	Fortify->Cost.Gold = 5;
	Fortify->Cost.Actions = 1;

	// NOTE: Unused
	ability *Aura = Abilities + Ability_Aura;
	Aura->Name = Ability_Aura;
	Aura->Type = AbilityType_Passive;
	Aura->Parameters[0] = 2;

	ability *Counter = Abilities + Ability_Counter;
	Counter->Name = Ability_Counter;
	Counter->Type = AbilityType_Reaction;
	Counter->Parameters[0] = 2;

	ability *GenerateFood = Abilities + Ability_GenerateFood;
	GenerateFood->Name = Ability_GenerateFood;
	GenerateFood->Type = AbilityType_Reaction;
	GenerateFood->Parameters[0] = 2;
	GenerateFood->Trigger = Reaction_TurnStart;

	ability *CreateResources = Abilities + Ability_CreateResources;
	CreateResources->Name = Ability_CreateResources;
	CreateResources->Type = AbilityType_Activated;
	CreateResources->Cooldown = 1;
	CreateResources->Parameters[0] = 1;
}

internal void
DamageEntity(game_state *GameState, game_mode_world *WorldMode, entity *Entity, s32 Damage)
{
	if((Entity->MaxDefense) && (Entity->Defense > 0))
	{
		Entity->Defense -= Damage;
		if(Entity->Defense <= 0)
		{
			for(u32 AbilityIndex = 0;
				AbilityIndex < Entity->AbilityCount;
				++AbilityIndex)
			{
				ability *Ability = Entity->Abilities + AbilityIndex;
				if((Ability->Type == AbilityType_Reaction) &&
				   (Ability->Trigger == Reaction_OnDestroy))
				{
					entity *Player = GetEntity(WorldMode->EntityManager, WorldMode->Players[Entity->Owner]);
					UseAbility(GameState, WorldMode, Player, Entity, Ability, GameState->UI.TargetIDs);
				}
			}

			if(!(Entity->Flags & EntityFlag_IsBuilding))
			{
				RemoveEntity(WorldMode->EntityManager, Entity);
			}
		}
	}
}

internal void
ChargeEntity(game_state *GameState, game_mode_world *WorldMode, entity *Entity, s32 Charges)
{
	s32 OldCharges = Entity->Charges;
	Entity->Charges += Charges;
	Entity->Charges = Maximum(Entity->Charges, 0);

	if(OldCharges != Entity->Charges)
	{
		for(u32 AbilityIndex = 0;
			AbilityIndex < Entity->AbilityCount;
			++AbilityIndex)
		{
			ability *Ability = Entity->Abilities + AbilityIndex;
			if((Ability->Type == AbilityType_Reaction) &&
			   (Ability->Trigger == Reaction_OnChargeChange))
			{
				entity *Player = GetEntity(WorldMode->EntityManager, WorldMode->Players[Entity->Owner]);
				UseAbility(GameState, WorldMode, Player, Entity, Ability, GameState->UI.TargetIDs);
			}
		}
	}
}

internal b32
AbilityCostsCanBePaid(game_mode_world *WorldMode, entity *Player, entity *Entity, ability *Ability)
{
	b32 Result = true;

	ability_cost Cost = Ability->Cost;
	if(Cost.Gold > Player->Gold) {Result = false;}
	if(Cost.Actions > Player->ActionsRemaining) {Result = false;}

	if(Cost.Charges > Entity->Charges) {Result = false;}

	if(Ability->CurrentCooldown < Ability->Cooldown) {Result = false;}

	return Result;
}

internal void
PayAbilityCosts(game_state *GameState, game_mode_world *WorldMode,
	entity *Player, entity *Entity, ability *Ability)
{
	ability_cost Cost = Ability->Cost;
	Player->Gold -= Cost.Gold;
	Player->ActionsRemaining -= Cost.Actions;
	ChargeEntity(GameState, WorldMode, Entity, -Cost.Charges);
	Ability->CurrentCooldown = 0;
}

internal void
RenderAbilityIcon(ui_state *UI, ability *Ability, v2i Min, v2i Max, f32 Alpha)
{
	render_group *Group = UI->Group;
	TemporaryClipRectChange(Group, Min, Max);
	TemporaryRenderFlagChange(Group,
		RenderFlag_SDF|RenderFlag_NotLighted|RenderFlag_AlphaBlended|RenderFlag_TopMost);

	v4 Colors[] =
	{
		V4(0.25f, 0.25f, 0.75f, 0.5f),
		V4(0.75f, 0.75f, 0.25f, 0.5f),
		V4(0.25f, 0.75f, 0.25f, 0.5f),
	};
	v4 Color = Colors[Ability->Type];
	Color.a = Alpha;
	PushQuad(Group, V2iToV2(Min), V2iToV2(Max), Color);

	rectangle2 LayoutRect = Rectangle2(V2iToV2(Min), V2iToV2(Max));
	ui_layout Layout = BeginUILayout(UI, Asset_TimesFont, 20.0f, V4(1,1,0,1), LayoutRect);
	BeginRow(&Layout);

	if(Ability->Cooldown)
	{
		s32 TurnsUntilReady = Maximum(Ability->Cooldown - Ability->CurrentCooldown, 0);
		Labelf(&Layout, "CD: %d  ", TurnsUntilReady);
	}

	Label(&Layout, Ability->DisplayName);

	EndRow(&Layout);
	EndUILayout(&Layout);
}

internal void
RenderAbilityInfo(ui_state *UI, ability *Ability, v2i Min, v2i Max)
{
	render_group *Group = UI->Group;

	TemporaryClipRectChange(Group, Min, Max);
	TemporaryRenderFlagChange(Group,
		RenderFlag_SDF|RenderFlag_NotLighted|RenderFlag_AlphaBlended|RenderFlag_TopMost);

	PushQuad(Group, V2iToV2(Min), V2iToV2(Max), V4(0.1f, 0.1f, 0.4f, 0.75f));

	rectangle2 LayoutRect = Rectangle2(V2iToV2(Min), V2iToV2(Max));
	ui_layout Layout = BeginUILayout(UI, Asset_TimesFont, 20.0f, V4(1,1,0.5f,1), LayoutRect);

	BeginRow(&Layout);
	Label(&Layout, Ability->DisplayName);
	EndRow(&Layout);

	BeginRow(&Layout);
	Layout.Color = V4(1,0.5f,1.0f,1);
	if(Ability->DescriptionFormat)
	{
		Labelf(&Layout, Ability->DescriptionFormat,
			Ability->Parameters[0],
			Ability->Parameters[1],
			Ability->Parameters[2],
			Ability->Parameters[3],
			Ability->Parameters[4],
			Ability->Parameters[5],
			Ability->Parameters[6],
			Ability->Parameters[7]);
	}
	else
	{
		Label(&Layout, WrapZ("Missing Description!"));
	}
	EndRow(&Layout);

	Layout.Color = V4(1,0.5f,0.5f,1);
	if(Ability->Cooldown)
	{
		BeginRow(&Layout);
		Labelf(&Layout, "Cooldown : %d", Ability->Cooldown);
		EndRow(&Layout);
	}

	if(Ability->TargetCount)
	{
		BeginRow(&Layout);
		Labelf(&Layout, "Target Count : %d", Ability->TargetCount);
		EndRow(&Layout);
	}

	if(Ability->TargetFlags & Target_OnBoard)
	{
		BeginRow(&Layout);
		Label(&Layout, WrapZ("Target : On Board"));
		EndRow(&Layout);
	}
	if(Ability->TargetFlags & Target_Traversable)
	{
		BeginRow(&Layout);
		Label(&Layout, WrapZ("Target : Tarversable"));
		EndRow(&Layout);
	}
	if(Ability->TargetFlags & Target_InHand)
	{
		BeginRow(&Layout);
		Label(&Layout, WrapZ("Target : In hand"));
		EndRow(&Layout);
	}
	if(Ability->TargetFlags & Target_Influenced)
	{
		BeginRow(&Layout);
		Label(&Layout, WrapZ("Target : Inside Influence"));
		EndRow(&Layout);
	}
	if(Ability->TargetFlags & Target_Building)
	{
		BeginRow(&Layout);
		Label(&Layout, WrapZ("Target : Building"));
		EndRow(&Layout);
	}

	if(Ability->Range)
	{
		BeginRow(&Layout);
		if(Ability->Range == S32Maximum)
		{
			Label(&Layout, WrapZ("Range : Anywhere"));
		}
		else
		{
			Labelf(&Layout, "Range : %d", Ability->Range);
		}
		EndRow(&Layout);
	}

	BeginRow(&Layout);
	Label(&Layout, WrapZ("Costs:"));
	EndRow(&Layout);

	BeginIndent(&Layout);
	if(Ability->Cost.Gold)
	{
		BeginRow(&Layout);
		Labelf(&Layout, "Gold : %d", Ability->Cost.Gold);
		EndRow(&Layout);
	}
	if(Ability->Cost.Charges)
	{
		BeginRow(&Layout);
		Labelf(&Layout, "Charges : %d", Ability->Cost.Charges);
		EndRow(&Layout);
	}
	if(Ability->Cost.Actions)
	{
		BeginRow(&Layout);
		Labelf(&Layout, "Actions : %d", Ability->Cost.Actions);
		EndRow(&Layout);
	}
	EndIndent(&Layout);

	BeginRow(&Layout);
	Label(&Layout, WrapZ(Names_ability_type[Ability->Type]));
	EndRow(&Layout);
	if(Ability->Type == AbilityType_Reaction)
	{
		BeginRow(&Layout);
		Label(&Layout, WrapZ(Names_reaction_trigger[Ability->Trigger]));
		EndRow(&Layout);
	}
	EndUILayout(&Layout);
}

internal b32
PositionIsInfluenced(game_mode_world *WorldMode, v2i BoardPosition, player_name Player)
{
	b32 Result = false;
	FOR_EACH(Entity, WorldMode->EntityManager, entity)
	{
		if((Entity->Influence) &&
		   (Entity->Owner == Player) &&
		   (Entity->Flags & EntityFlag_OnBoard))
		{
			s32 Distance = HexDistance(BoardPosition, Entity->BoardPosition);
			if(Distance <= Entity->Influence)
			{
				Result = true;
				break;
			}
		}
	}

	return Result;
}

internal b32
TargetIsValid(game_mode_world *WorldMode, entity *CastingEntity, ability *Ability, entity *Target)
{
	b32 Result = true;
	u32 Flags = Ability->TargetFlags;
	if(Flags & Target_OnBoard)
	{
		if(!(Target->Flags & EntityFlag_OnBoard)) {Result = false;}
	}
	if(Flags & Target_InHand)
	{
		if(!(Target->Flags & EntityFlag_InHand)) {Result = false;}
	}
	if(Flags & Target_InRange)
	{
		s32 Distance = EntityHexDistance(CastingEntity, Target);
		if(Distance > Ability->Range) {Result = false;}
	}
	if(Flags & Target_Traversable)
	{
		if(!TileIsTraversable(WorldMode, Target->BoardPosition))
		{
			Result = false;
		}
	}
	if(Flags & Target_Influenced)
	{
		if(!PositionIsInfluenced(WorldMode, Target->BoardPosition, CastingEntity->Owner))
		{
			Result = false;
		}
	}
	if(Flags & Target_Building)
	{
		if(!(Target->Flags & EntityFlag_IsBuilding)) {Result = false;}
	}
	if(Flags & Target_Unit)
	{
		if(!(Target->Flags & EntityFlag_IsUnit)) {Result = false;}
	}

	return Result;
}

internal void
DrawRandomCard(game_state *GameState, game_mode_world *WorldMode, entity *Player)
{
	deck *Deck = WorldMode->Decks + Player->Owner;
	u32 Choice = RandomChoice(&WorldMode->Entropy, Deck->CardCount);
	ability_name AbilityName = Deck->Cards[Choice];
	Deck->Cards[Choice] = Deck->Cards[--Deck->CardCount];
	AddCard(GameState, WorldMode, Player->Owner, AbilityName);
}

internal b32
UseAbility(game_state *GameState, game_mode_world *WorldMode,
	entity *Player, entity *CastingEntity, ability *Ability,
	entity_id *TargetIDs)
{
	b32 AbilityUsed = false;

	b32 CanPayCosts = AbilityCostsCanBePaid(WorldMode, Player, CastingEntity, Ability);
	if(CanPayCosts)
	{
		entity *Targets[MAX_TARGETS] = {};
		for(u32 TargetIndex = 0;
			TargetIndex < MAX_TARGETS;
			++TargetIndex)
		{
			Targets[TargetIndex] = GetEntity(WorldMode->EntityManager, TargetIDs[TargetIndex]);
		}

		PayAbilityCosts(GameState, WorldMode, Player, CastingEntity, Ability);
		AbilityUsed = true;

		switch(Ability->Name)
		{
			case Ability_Move:
			{
				Assert(Targets[0]->Flags & EntityFlag_OnBoard);
				v3 P = Targets[0]->P;
				v2i BoardPosition = Targets[0]->BoardPosition;
				Assert(TileIsTraversable(WorldMode, BoardPosition));

				PushInterpAnimation(&WorldMode->Animator, CastingEntity, P - CastingEntity->P,
					CastingEntity->Rotation, 0.25f, Interp_Cubic);
				CastingEntity->BoardPosition = BoardPosition;
			} break;

			case Ability_Attack:
			{
				v3 P = Targets[0]->P;
				SpawnFire(WorldMode->ParticleSystem, P);

				entity_list List = GetEntitiesAtPosition(&WorldMode->Board, Targets[0]->BoardPosition);
				for(u32 EntityIndex = 0;
					EntityIndex < List.EntityCount;
					++EntityIndex)
				{
					entity *Entity = GetEntity(WorldMode->EntityManager, List.EntityIDs[EntityIndex]);
					DamageEntity(GameState, WorldMode, Entity, CastingEntity->Attack);
				}
			} break;

			case Ability_GenerateCharge:
			{
				ChargeEntity(GameState, WorldMode, CastingEntity, Ability->Parameters[0]);
			} break;

			case Ability_BuildHeadquarters:
			{
				v2i BoardPosition = Targets[0]->BoardPosition;
				CreateEntityOnBoard(WorldMode, WorldMode->EntityManager, GameState->Assets,
					Entity_Headquarters, BoardPosition, Player->Owner);
			} break;

			case Ability_DrawCard:
			{
				DrawRandomCard(GameState, WorldMode, Player);
			} break;

			case Ability_GainGold:
			{
				Player->Gold += Ability->Parameters[0];
			} break;

			case Ability_GatherResources:
			{
				s32 ResourceRange = Ability->Parameters[0];
				FOR_EACH(Entity, WorldMode->EntityManager, entity)
				{
					if(EntityHexDistance(CastingEntity, Entity) <= ResourceRange)
					{
						Player->Gold += Entity->Gold;
					}
				}
			} break;

			case Ability_HQPillaged:
			{
				entity *Opponent = GetOpponent(WorldMode, Player);
				Opponent->VictoryPoints += Ability->Parameters[0];
			} break;

			case Ability_BuildBank:
			{
				v2i BoardPosition = Targets[0]->BoardPosition;
				entity *Bank = CreateEntityOnBoard(WorldMode, WorldMode->EntityManager, GameState->Assets,
					Entity_Bank, BoardPosition, Player->Owner);

				Bank->Charges = Player->Gold;
				Player->Gold = 0;
			} break;

			case Ability_BankActive:
			{
				Player->Gold += Ability->Parameters[0];
			} break;

			case Ability_BankPillaged:
			{
				if(CastingEntity->Charges > 0)
				{
					ChargeEntity(GameState, WorldMode, CastingEntity, -1);
					entity *Opponent = GetOpponent(WorldMode, Player);
					Opponent->Gold += Ability->Parameters[0];
				}
			} break;

			case Ability_BuildRiskInvestment:
			{
				v2i BoardPosition = Targets[0]->BoardPosition;
				entity *RiskInvestment = CreateEntityOnBoard(WorldMode, WorldMode->EntityManager,
					GameState->Assets, Entity_RiskInvestment, BoardPosition, Player->Owner);
			} break;

			case Ability_RiskInvestmentReturn:
			{
				if(CastingEntity->Charges >= Ability->Parameters[0])
				{
					Player->Gold += Ability->Parameters[1];
					RemoveEntity(WorldMode->EntityManager, CastingEntity);
				}
			} break;

			case Ability_RiskInvestmentPillaged:
			{
				RemoveEntity(WorldMode->EntityManager, CastingEntity);
			} break;

			case Ability_BuildGoldMine:
			{
				v2i BoardPosition = Targets[0]->BoardPosition;
				entity *GoldMine = CreateEntityOnBoard(WorldMode, WorldMode->EntityManager, GameState->Assets,
					Entity_GoldMine, BoardPosition, Player->Owner);
			} break;

			case Ability_GoldMine:
			{
				Player->Gold += Ability->Parameters[0];
			} break;

			case Ability_GoldMinePillaged:
			{
				u32 GoldLost = Minimum(Player->Gold, Ability->Parameters[0]);
				Player->Gold -= GoldLost;
			} break;

			case Ability_BuildPowerPlant:
			{
				v2i BoardPosition = Targets[0]->BoardPosition;
				entity *PowerPlant = CreateEntityOnBoard(WorldMode, WorldMode->EntityManager, GameState->Assets, Entity_PowerPlant, BoardPosition, Player->Owner);
			} break;

			case Ability_GeneratePower:
			{
				s32 PowerRange = Ability->Parameters[0];
				FOR_EACH(Entity, WorldMode->EntityManager, entity)
				{
					if(EntityHexDistance(CastingEntity, Entity) <= PowerRange)
					{
						ChargeEntity(GameState, WorldMode, Entity, Ability->Parameters[1]);
					}
				}
			} break;

			case Ability_PowerPlantPillaged:
			{
				entity *Opponent = GetOpponent(WorldMode, Player);
				CastingEntity->Owner = Opponent->Owner;
			} break;

			case Ability_SummonWarrior:
			{
				v2i BoardPosition = Targets[0]->BoardPosition;
				CreateEntityOnBoard(WorldMode, WorldMode->EntityManager, GameState->Assets,
					Entity_Warrior, BoardPosition, Player->Owner);
			} break;

			case Ability_Fireball:
			{
				v3 P = Targets[0]->P;
				SpawnFire(WorldMode->ParticleSystem, P, 20);

				entity_list List = GetEntitiesAtPosition(&WorldMode->Board, Targets[0]->BoardPosition);
				for(u32 EntityIndex = 0;
					EntityIndex < List.EntityCount;
					++EntityIndex)
				{
					entity *Entity = GetEntity(WorldMode->EntityManager, List.EntityIDs[EntityIndex]);
					DamageEntity(GameState, WorldMode, Entity, Ability->Parameters[0]);
				}
			} break;

			case Ability_SummonMercenaries:
			{
				v2i BoardPosition = Targets[0]->BoardPosition;
				entity *Mercs = CreateEntityOnBoard(WorldMode, WorldMode->EntityManager, GameState->Assets, Entity_Mercenaries, BoardPosition, Player->Owner);
			} break;

			case Ability_HireMercenaries:
			{
				CastingEntity->Attack += 2;
			} break;

			case Ability_HedgeFund:
			{
				Player->Gold += Ability->Parameters[0];
			} break;

			case Ability_Buyout:
			{
				Targets[0]->Owner = Player->Owner;
			} break;

			case Ability_Bribe:
			{
				Targets[0]->Owner = Player->Owner;
			} break;

			case Ability_Fortify:
			{
				Targets[0]->MaxDefense += Ability->Parameters[0];
				Targets[0]->Defense += Ability->Parameters[0];
			} break;

			case Ability_CreateResources:
			{
				Player->Gold += Ability->Parameters[0];
			} break;

			case Ability_GenerateFood:
			{
				// Player->Food += Ability->Parameters[0];
			} break;

			InvalidDefaultCase;
		}

		if(CastingEntity->Flags & EntityFlag_InHand)
		{
			RemoveEntity(WorldMode->EntityManager, CastingEntity);
		}
	}

	return AbilityUsed;
}

