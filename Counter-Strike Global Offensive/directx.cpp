//#include "hooked.hpp"
//#include "displacement.hpp"
//#include "player.hpp"
//#include "weapon.hpp"
//#include "prediction.hpp"
//#include "movement.hpp"
//#include "aimbot.hpp"
//#include "anti_aimbot.hpp"
//#include "menu.h"
//#include "game_movement.h"
//#include "lag_compensation.hpp"
//#include <intrin.h>
//#include "legitbot.hpp"
//#include "md5_shit.h"
//#include "renderer.hpp"
//
//long __stdcall Hooked::EndScene(IDirect3DDevice9* device)
//{
//	using Fn = long(__stdcall*)(IDirect3DDevice9* device);
//	auto original = Source::m_pDeviceSwap->VCall<Fn>(Index::IDirectX::EndScene);
//
//	if (!original)
//		return 0;
//
//	static void* allowed_ret_addr = nullptr; if (!allowed_ret_addr) allowed_ret_addr = _ReturnAddress();
//	if (allowed_ret_addr != _ReturnAddress())
//		return original(device);
//
//	IDirect3DStateBlock9* pixel_state = NULL; IDirect3DVertexDeclaration9* vertDec; IDirect3DVertexShader9* vertShader;
//	device->CreateStateBlock(D3DSBT_PIXELSTATE, &pixel_state);
//	device->GetVertexDeclaration(&vertDec);
//	device->GetVertexShader(&vertShader);
//
//	//renderer::run(device);
//
//	pixel_state->Apply();
//	pixel_state->Release();
//	device->SetVertexDeclaration(vertDec);
//	device->SetVertexShader(vertShader);
//
//	return original(device);
//}
//
//long __stdcall Hooked::Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pp)
//{
//	using Fn = long(__stdcall*)(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pp);
//	auto original = Source::m_pDeviceSwap->VCall<Fn>(Index::IDirectX::Reset);
//
//	if (!original)
//		return 0;
//
//	//renderer::lost(device);
//	auto result = original(device, pp);
//	//renderer::create(device);
//
//	return result;
//}