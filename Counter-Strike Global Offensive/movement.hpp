#pragma once

#include "sdk.hpp"

namespace Engine
{

class Movement : public Core::Singleton<Movement>
{
public:
	float get_move_angle(float speed);;
	void predict_velocity(Vector * velocity);
	void RotateMovement(CUserCmd * cmd, float yaw);
	void quick_stop(CUserCmd * cmd);
	void Begin( CUserCmd* cmd, bool& send_packet);
	void End();

	QAngle m_qRealAngles = {};
	QAngle m_qAnglesView = {};
private:
	CUserCmd* m_pCmd = nullptr;
	C_CSPlayer* m_pPlayer = nullptr;

	QAngle m_qAngles = { };
	QAngle m_qAnglesLast = { };
};

}