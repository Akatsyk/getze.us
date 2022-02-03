#pragma once

#include "entity.hpp"

struct sm_animdata_t
{
	Vector m_lastOrigin;
	Vector m_lastVelocity;
	Vector m_animVelocity;
	bool m_filled;
};

enum Indexes
{
	GET_ABS_ORIGIN = 10,
	GET_ABS_ANGLES = 11,
	IS_SELF_ANIMATING = 155,
	SET_MODEL = 9,
	UPDATE_CLIENTSIDE_ANIMATIONS = 223, //219
};

enum DataUpdateType_t
{
	DATA_UPDATE_CREATED = 0,
	DATA_UPDATE_DATATABLE_CHANGED,
};

class C_BasePlayer : public C_BaseCombatCharacter
{
public:
	QAngle& m_aimPunchAngle();
	QAngle & m_aimPunchAngleVel();
	QAngle& m_viewPunchAngle();
	Vector& m_vecViewOffset();
	float& m_flDuckAmount();
	float & m_flTimeOfLastInjury();
	float & m_flDuckSpeed();
	datamap_t * GetPredDescMap();
	float & m_flStepSize();
	bool & m_bGunGameImmunity();
	bool & m_bStrafing();
	Vector & m_datavecBaseVelocity();
	float & m_flMaxspeed();
	unsigned int FindInDataMap(datamap_t * pMap, const char * name);
	float & m_surfaceFriction();
	C_WeaponCSBaseGun * get_weapon();
	void CreateAnimationState(CCSGOPlayerAnimState * state);
	float m_flSpawnTime();
	float & m_flNextAttack();
	Vector get_bone_pos(int iBone);
	void modify_eye_pos(CCSGOPlayerAnimState * animstate, Vector * pos);
	void eye_pos(Vector * piska);
	Vector GetEyePosition();
	CBaseHandle *m_hMyWearables();
	bool IsBot();
	int & LastBoneSetupFrame();
	void update_client_anims();
	//float&m_flLastShotTime();
	void update_clientside_animations();
	matrix3x4_t ** dwBoneMatrix();
	CUtlVector<matrix3x4_t>& m_CachedBoneData();
	int & GetBoneCount();
	matrix3x4_t * dwBoneMatrixptr();
	matrix3x4_t m_mxBoneMatrix(int BoneID);
	CBoneAccessor * GetBoneAccessor();
	CStudioHdr * GetModelPtr();
	void StandardBlendingRules(CStudioHdr * hdr, Vector * pos, Quaternion * q, float curtime, int32_t boneMask);
	void set_model_index(int index);
	int &m_iShotsFired();
	bool & m_bDucking();
	bool & m_bDucked();
	float & m_flLastDuckTime();
	void BuildTransformations(CStudioHdr * hdr, Vector * pos, Quaternion * q, const matrix3x4_t & cameraTransform, int32_t boneMask, byte * computed);
	void force_bone_rebuild();
	void break_setup_bones();
	bool setup_bones_broken();
	bool & client_side_animation();
	void pre_data_update(int updateType);
	void post_data_update(int updateType);
	bool & is_animating();
	Vector & get_abs_origin();
	QAngle & get_abs_eye_angles();
	QAngle & get_eye_angles();
	float get_fixed_curtime(CUserCmd * cmd);
	void set_abs_origin(Vector origin);
	void UpdateVisibilityAllEntities();
	int draw_model(int flags, uint8_t alpha);
	int & m_nSequence();
	float & m_flLowerBodyYawTarget();
	std::array<float, 24>& m_flPoseParameter();
	float * m_flPoseParameter_ptr();
	float & get_pose(int index);
	int & m_nHitboxSet();
	int & m_nBody();
	Vector & m_vecRagdollOrigin();
	float get_bomb_blow_timer();
	bool m_bHasDefuser();
	int &m_nExplodeEffectTickBegin();
	float & m_flHealthShotBoostExpirationTime();
	C_BasePlayer * m_hBombDefuser();
	float get_bomb_defuse_timer();
	float & m_flDefuseCountDown();
	int & m_nSkin();
	int & m_hOwnerEntity();
	int & m_nSmokeEffectTickBegin();
	matrix3x4_t & GetCollisionBoundTrans();
	bool & m_bHasHelmet();
	bool & m_bHasHeavyArmor();
	QAngle & m_angEyeAngles();
	int get_sec_activity(int sequence);
	int & TakeDamage();
	std::string GetSteamID();
	int & m_ArmorValue();
	float & m_flFlashMaxAlpha();
	float & m_flFlashDuration();
	float & m_flFlashTime();
	float & m_flPlaybackRate();
	uint8_t * GetServerEdict();
	bool ComputeHitboxSurroundingBox(matrix3x4_t * mx,Vector * pVecWorldMins, Vector * pVecWorldMaxs);
	float GetSequenceMoveDist(CStudioHdr * pStudioHdr, int iSequence);
	float SequenceDuration(CStudioHdr * pStudioHdr, int iSequence);
	void DrawServerHitboxes();
	bool SetupBonesEx();
	QAngle * m_angEyeAngles_ptr();
	float & m_flCycle();
	std::string  m_szNickName();
	void set_abs_angles(QAngle origin);
	HANDLE m_hViewModel();
	CBaseHandle * m_hObserverTarget();
	bool& m_bWaitForNoAttack();
	void invalidate_anims();
	float & m_flSimulationTime();
	float & m_flAnimTime();
	QAngle & get_render_angles();
	CCSGOPlayerAnimState * get_animation_state();
	bool get_multipoints(int ihitbox, std::vector<Vector>& points, matrix3x4_t mx[]);
	void * animation_layers_ptr();
	C_AnimationLayer & animation_layer(int i);
	C_AnimationLayer & get_animation_layer(int index, bool eblan = false);
	int get_animation_layers_count();
	Vector& m_vecVelocity();
	int & m_iEFlags();
	Vector & m_vecAbsVelocity();
	float & m_flOldSimulationTime();
	void calc_anim_velocity(sm_animdata_t* sm_animdata, bool reset);
	//void calc_anim_velocity(bool reset);
	Vector& m_vecBaseVelocity();
	float& m_flFallVelocity();
	char& m_lifeState();
	float & m_flVelocityModifier();
	int & m_hGroundEntity();
	int& m_nTickBase();
	int& m_iHealth();
	int& m_fFlags();
	bool& m_bIsScoped();
	const char * m_szLastPlaceName();
	int * m_hMyWeapons();
	float m_anim_time();
	void UpdateAnimationState(CCSGOPlayerAnimState * state, QAngle angle);
	void SetupVelocity(CCSGOPlayerAnimState * state);
	std::vector<Vector> build_capsules(Vector Min, Vector Max, float Radius, float scale, matrix3x4_t & Transform);
	int & OcculusionFrame();
	int &OcclusionMask();
	bool SetupFuckingBones(matrix3x4_t * pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime, QAngle* angles_override = nullptr);

	bool IsDead();
	void SetCurrentCommand( CUserCmd* cmd );
};

class C_BaseViewModel : public C_BasePlayer
{
public:
	CBaseHandle* get_weapon();
	float& m_flCycle();
	float& m_flAnimTime();
	int& m_nSequence();
	void send_matching_seq(int seq);
};

class C_CSPlayer : public C_BasePlayer
{
public:
	static C_CSPlayer* GetLocalPlayer();
public:
	QAngle& m_angEyeAngles();
};

FORCEINLINE C_CSPlayer* ToCSPlayer( C_BaseEntity* pEnt )
{
	if( !pEnt || !pEnt->IsPlayer() )
		return nullptr;

	return ( C_CSPlayer* )( pEnt );
}

class CRecipientFilter
{
public:

	CRecipientFilter() {
		m_Recipients.RemoveAll();
		m_bReliable = false;
	};
	~CRecipientFilter() {
		m_Recipients.RemoveAll();
		m_bReliable = false;
	};

	void AddRecipient(C_BasePlayer *player)
	{
		Assert(player);

		if (!player)
			return;

		int index = player->entindex();

		//// If we're predicting and this is not the first time we've predicted this sound
		////  then don't send it to the local player again.
		//if (m_bUsingPredictionRules)
		//{
		//	// Only add local player if this is the first time doing prediction
		//	if (g_RecipientFilterPredictionSystem.GetSuppressHost() == player)
		//	{
		//		return;
		//	}
		//}

		// Already in list
		if (m_Recipients.Find(index) != m_Recipients.InvalidIndex())
			return;

		m_Recipients.AddToTail(index);
	}

	void MakeReliable(void)
	{
		m_bReliable = true;
	}

	bool				m_bReliable;
	bool				m_bInitMessage;
	CUtlVector< int >	m_Recipients;

	// If using prediction rules, the filter itself suppresses local player
	bool				m_bUsingPredictionRules;
	// If ignoring prediction cull, then external systems can determine
	//  whether this is a special case where culling should not occur
	bool				m_bIgnorePredictionCull;
};

class CSingleUserRecipientFilter : public CRecipientFilter
{
public:
	CSingleUserRecipientFilter(C_BasePlayer *player)
	{
		AddRecipient(player);
	}
};
