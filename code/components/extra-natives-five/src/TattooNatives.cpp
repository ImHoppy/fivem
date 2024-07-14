#include "StdInc.h"

#include <EntitySystem.h>

#include <Local.h>
#include <Hooking.h>
#include <ScriptEngine.h>
#include <nutsnbolts.h>
#include "NativeWrappers.h"

#include "atArray.h"

#include <GameInit.h>
#include <fxScripting.h>
#include <Resource.h>
#include <StdInc.h>

#include <GameInit.h>
#include <CoreConsole.h>

#include <Hooking.h>

#include <scrEngine.h>

struct Vector2
{
	float x;
	float y;

	Vector2(float x, float y, float z, float w)
		: x(x), y(y)
	{
	}
};

enum ePedDecorationZone
{
	ZONE_TORSO = 0,
	ZONE_HEAD,
	ZONE_LEFT_ARM,
	ZONE_RIGHT_ARM,
	ZONE_LEFT_LEG,
	ZONE_RIGHT_LEG,
	ZONE_MEDALS,
	ZONE_INVALID,
};

enum ePedDecorationType
{
	TYPE_TATTOO = 0,
	TYPE_BADGE = 1,
	TYPE_MEDAL = 2,
	TYPE_INVALID,
};

enum Gender
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_DONTCARE
};

class PedDecorationPreset
{
public:
	constexpr static size_t kSize = 0x40;

public:
	Vector2 m_uvPos;
	Vector2 m_scale;
	float m_rotation;
	uint32_t m_nameHash;
	uint32_t m_txdHash;
	uint32_t m_txtHash;
	ePedDecorationZone m_zone;
	ePedDecorationType m_type;

	uint32_t m_faction; // human readable version of the faction name
	uint32_t m_garment; // human readable version of the garment id
	Gender m_gender; // gender just used for UI/sorting
	uint32_t m_award; // human readable version of the reward name
	uint32_t m_awardLevel;

	bool m_usesTintColor;
};

class PedDecorationCollection
{
public:
	constexpr static size_t kSize = 0xA0; // Class definition has never changed.

	atArray<PedDecorationPreset> m_presets;
	uint32_t m_nameHash;

	Vector2 unk[16];
	Vector2 unk1;
	bool unk2;

	inline uint32_t GetName() const
	{
		return this->m_nameHash;
	}

	inline PedDecorationPreset* GetPreset(int index) 
	{
		this->m_presets[index];
		auto array = *reinterpret_cast<char**>((char*)this + (ptrdiff_t)0);
		return reinterpret_cast<PedDecorationPreset*>(array + index * PedDecorationPreset::kSize);
	}
};

class PedDecorationManager
{
public:
	inline static ptrdiff_t kCollectionsOffset;
	inline static PedDecorationManager** ms_instance;
	

	uint64_t unk[22]; // PedDecalDecorationManager
	atArray<PedDecorationCollection> m_collections;

public:
	inline PedDecorationCollection* GetCollection(size_t index) const
	{
		auto array = *reinterpret_cast<char**>((char*)this + kCollectionsOffset);
		return reinterpret_cast<PedDecorationCollection*>(array + index * PedDecorationCollection::kSize);
	}

	static PedDecorationManager* GetInstance()
	{
		return *ms_instance;
	}

	static int Comparator(const void* a, const void* b)
	{
		auto instance = GetInstance();
		uint32_t a_hash = instance->GetCollection(*static_cast<const uint32_t*>(a))->GetName();
		uint32_t b_hash = instance->GetCollection(*static_cast<const uint32_t*>(b))->GetName();
		return (a_hash > b_hash) - (a_hash < b_hash);
	}
};

static HookFunction hookFunction([]()
{
	{
		auto location = hook::get_pattern<char>("41 0F B7 DE 4C 8D 0D ? ? ? ? 41 B8");

		auto compare = hook::get_address<char*>(location + 0x7);
		PedDecorationManager::ms_instance = hook::get_address<PedDecorationManager**>(compare + 0x3);
		PedDecorationManager::kCollectionsOffset = *reinterpret_cast<int32_t*>(compare + 0x7 + 0x3);
	}

	fx::ScriptEngine::RegisterNativeHandler("GET_TATTOO_AT_INDEX", [=](fx::ScriptContext& context)
	{

		bool result = false;

		int collectionIndex = context.GetArgument<int>(0);
		int tattooIndex = context.GetArgument<int>(1);


		if (collectionIndex >= 0)
		{
			PedDecorationCollection* collection = (*PedDecorationManager::ms_instance)->GetCollection(collectionIndex);
			if (collection != nullptr)
			{
				PedDecorationPreset* preset = collection->GetPreset(tattooIndex);
				if (preset != nullptr)
				{
					*context.GetArgument<int*>(2) = preset->m_gender;

					scrVector* m_uvPos = context.GetArgument<scrVector*>(3);
					scrVector* m_scale = context.GetArgument<scrVector*>(4);
					auto copyVector = [](const Vector2 in, scrVector* out)
					{
						out->x = in.x;
						out->y = in.y;
						out->z = 0;
					};

					copyVector(preset->m_uvPos, m_uvPos);
					copyVector(preset->m_scale, m_scale);

					*context.GetArgument<int*>(5) = preset->m_rotation;
					*context.GetArgument<int*>(6) = preset->m_nameHash;
					*context.GetArgument<int*>(7) = preset->m_txdHash;
					*context.GetArgument<int*>(8) = preset->m_txtHash;
					// ePedDecorationZone m_zone;
					// ePedDecorationType m_type;
					/*
					 *context.GetArgument<int*>(3) = data->overlayColorType[index];
					 *context.GetArgument<int*>(4) = data->overlayColorId[index];
					 *context.GetArgument<int*>(5) = data->overlayHighlightId[index];
					 *context.GetArgument<float*>(6) = data->overlayAlpha[index];*/

					result = true;
				}
			}
		}

		context.SetResult<bool>(result);
	});
});
