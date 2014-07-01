#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/FS/vfsDir.h"
#include "Crypto/unedat.h"
#include "sceNp.h"

//void sceNp_init();
//Module sceNp(0x0016, sceNp_init);
Module *sceNp = nullptr;

int sceNpInit(u32 mem_size, u32 mem_addr)
{
	sceNp->Log("sceNpInit(mem_size=0x%x, mem_addr=0x%x)", mem_size, mem_addr);
	return CELL_OK;
}

int sceNpTerm()
{
	sceNp->Log("sceNpTerm");
	return CELL_OK;
}

int sceNpDrmIsAvailable(u32 k_licensee_addr, u32 drm_path_addr)
{
	sceNp->Warning("sceNpDrmIsAvailable(k_licensee_addr=0x%x, drm_path_addr=0x%x)", k_licensee_addr, drm_path_addr);

	std::string drm_path = Memory.ReadString(drm_path_addr);
	if (!Emu.GetVFS().ExistsFile(drm_path))
	{
		sceNp->Warning("sceNpDrmIsAvailable(): '%s' not found", drm_path.c_str());
		return CELL_ENOENT;
	}

	std::string k_licensee_str;
	u8 k_licensee[0x10];
	for(int i = 0; i < 0x10; i++)
	{
		k_licensee[i] = Memory.Read8(k_licensee_addr + i);
		k_licensee_str += fmt::Format("%02x", k_licensee[i]);
	}

	sceNp->Warning("sceNpDrmIsAvailable: Found DRM license file at %s", drm_path.c_str());
	sceNp->Warning("sceNpDrmIsAvailable: Using k_licensee 0x%s", k_licensee_str.c_str());

	// Set the necessary file paths.
	std::string drm_file_name = fmt::AfterLast(drm_path,'/');

	//make more explicit what this actually does (currently everything after the third slash and before the fourth slash)
	std::string titleID = fmt::BeforeFirst(fmt::AfterFirst(fmt::AfterFirst(fmt::AfterFirst(drm_path,'/'),'/'),'/'),'/');

	std::string enc_drm_path = rGetCwd() + drm_path;
	std::string dec_drm_path = rGetCwd() + "/dev_hdd1/" + titleID + "/" + drm_file_name;

	std::string rap_dir_path = rGetCwd() + "/dev_usb000/";
	std::string rap_file_path = rap_dir_path;
	
	// Search dev_usb000 for a compatible RAP file. 
	vfsDir *raps_dir = new vfsDir(rap_dir_path);
	if (!raps_dir->IsOpened())
		sceNp->Warning("sceNpDrmIsAvailable: Can't find RAP file for DRM!");
	else
	{
		const std::vector<DirEntryInfo> &entries = raps_dir->GetEntries();
		for (auto &entry:  entries)
		{
			if (entry.name.find(titleID) != std::string::npos )
			{
				rap_file_path += entry.name;
				break;
			}
		}
	}

	// Create a new directory under dev_hdd1/titleID to hold the decrypted data.
	std::string tmp_dir = rGetCwd() + "/dev_hdd1/" + titleID;
	if (!rDir::Exists(tmp_dir))
		rMkdir(rGetCwd() + "/dev_hdd1/" + titleID);

	// Decrypt this EDAT using the supplied k_licensee and matching RAP file.
	DecryptEDAT(enc_drm_path, dec_drm_path, 8, rap_file_path, k_licensee, false);

	return CELL_OK;
}

int sceNpDrmIsAvailable2(u32 k_licensee_addr, u32 drm_path_addr)
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmVerifyUpgradeLicense(u32 content_id_addr)
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmVerifyUpgradeLicense2(u32 content_id_addr)
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmExecuteGamePurchase()
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpDrmGetTimelimit(u32 drm_path_addr, mem64_t time_remain_usec)
{
	UNIMPLEMENTED_FUNC(sceNp);
	return CELL_OK;
}

int sceNpManagerGetStatus(mem32_t status)
{
	sceNp->Log("sceNpManagerGetStatus(status_addr=0x%x)", status.GetAddr());

	// TODO: Check if sceNpInit() was called, if not return SCE_NP_ERROR_NOT_INITIALIZED
	if (!status.IsGood())
		return SCE_NP_ERROR_INVALID_ARGUMENT;

	// TODO: Support different statuses
	status = SCE_NP_MANAGER_STATUS_OFFLINE;
	return CELL_OK;
}

void sceNp_init()
{
	sceNp->AddFunc(0xbd28fdbf, sceNpInit);
	sceNp->AddFunc(0x4885aa18, sceNpTerm);
	sceNp->AddFunc(0xad218faf, sceNpDrmIsAvailable);
	sceNp->AddFunc(0xf042b14f, sceNpDrmIsAvailable2);
	sceNp->AddFunc(0x2ecd48ed, sceNpDrmVerifyUpgradeLicense);
	sceNp->AddFunc(0xbe0e3ee2, sceNpDrmVerifyUpgradeLicense2);
	sceNp->AddFunc(0xf283c143, sceNpDrmExecuteGamePurchase);
	sceNp->AddFunc(0xcf51864b, sceNpDrmGetTimelimit);
	sceNp->AddFunc(0xa7bff757, sceNpManagerGetStatus);
}
