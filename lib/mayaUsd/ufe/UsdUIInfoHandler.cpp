//
// Copyright 2020 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "UsdUIInfoHandler.h"
#include "UsdSceneItem.h"

#if UFE_PREVIEW_VERSION_NUM >= 2023
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/pcp/types.h>
#endif

#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>

#include <map>
#include <set>

MAYAUSD_NS_DEF {
namespace ufe {

UsdUIInfoHandler::UsdUIInfoHandler()
	: Ufe::UIInfoHandler()
{}

UsdUIInfoHandler::~UsdUIInfoHandler()
{
}

/*static*/
UsdUIInfoHandler::Ptr UsdUIInfoHandler::create()
{
	return std::make_shared<UsdUIInfoHandler>();
}

//------------------------------------------------------------------------------
// Ufe::UIInfoHandler overrides
//------------------------------------------------------------------------------

bool UsdUIInfoHandler::treeViewCellInfo(const Ufe::SceneItem::Ptr& item, Ufe::CellInfo& info) const
{
	bool changed = false;
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	if (usdItem)
	{
		if (!usdItem->prim().IsActive())
		{
			changed = true;
			info.fontStrikeout = true;
			MDoubleArray outlinerInvisibleColor;
			if (MGlobal::executeCommand("displayRGBColor -q \"outlinerInvisibleColor\"", outlinerInvisibleColor) &&
				(outlinerInvisibleColor.length() == 3))
			{
				double rgb[3];
				outlinerInvisibleColor.get(rgb);
				info.textFgColor.set(static_cast<float>(rgb[0]), static_cast<float>(rgb[1]), static_cast<float>(rgb[2]));
			}
			else
			{
				info.textFgColor.set(0.403922f, 0.403922f, 0.403922f);
			}
		}
	}

	return changed;
}

#if UFE_PREVIEW_VERSION_NUM >= 2023
Ufe::UIInfoHandler::Icon UsdUIInfoHandler::treeViewIcon(const Ufe::SceneItem::Ptr& item) const
#else
std::string UsdUIInfoHandler::treeViewIcon(const Ufe::SceneItem::Ptr& item) const
#endif
{
	// Special case for nullptr input.
	if (!item) {
#if UFE_PREVIEW_VERSION_NUM >= 2023
		return Ufe::UIInfoHandler::Icon("out_USD_UsdTyped.png");	// Default USD icon
#else
		return "out_USD_UsdTyped.png";	// Default USD icon
#endif
	}

	// We support these node types directly.
	static const std::map<std::string, std::string> supportedTypes{
		{"",					"out_USD_Def.png"},					// No node type
		{"BlendShape",			"out_USD_BlendShape.png"},
		{"Camera",				"out_USD_Camera.png"},
		{"Capsule",				"out_USD_Capsule.png"},
		{"Cone",				"out_USD_Cone.png"},
		{"Cube",				"out_USD_Cube.png"},
		{"Cylinder",			"out_USD_Cylinder.png"},
		{"GeomSubset",			"out_USD_GeomSubset.png"},
		{"LightFilter",			"out_USD_LightFilter.png"},
		{"LightPortal",			"out_USD_LightPortal.png"},
		{"mayaReference",		"out_USD_mayaReference.png"},
		{"AL_MayaReference",	"out_USD_mayaReference.png"},		// Same as mayaRef
		{"Mesh",				"out_USD_Mesh.png"},
		{"NurbsPatch",			"out_USD_NurbsPatch.png"},
		{"PointInstancer",		"out_USD_PointInstancer.png"},
		{"Points",				"out_USD_Points.png"},
		{"Scope",				"out_USD_Scope.png"},
		{"SkelAnimation",		"out_USD_SkelAnimation.png"},
		{"Skeleton",			"out_USD_Skeleton.png"},
		{"SkelRoot",			"out_USD_SkelRoot.png"},
		{"Sphere",				"out_USD_Sphere.png"},
		{"Volume",				"out_USD_Volume.png"}
	};

#if UFE_PREVIEW_VERSION_NUM >= 2023
	Ufe::UIInfoHandler::Icon icon;		// Default is empty (no icon and no badge).
#endif

	const auto search = supportedTypes.find(item->nodeType());
	if (search != supportedTypes.cend()) {
#if UFE_PREVIEW_VERSION_NUM >= 2023
		icon.baseIcon = search->second;
#else
		return search->second;
#endif
	}

#if UFE_PREVIEW_VERSION_NUM >= 2023
	// Check if we have any composition arcs - if yes we display a special badge.
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
	if (usdItem)
	{
		PXR_NS::UsdPrimCompositionQuery query(usdItem->prim());
		for (const auto& arc : query.GetCompositionArcs())
		{
			bool keepLooking = true;
			switch (arc.GetArcType())
			{
			case PXR_NS::PcpArcTypeReference:
			case PXR_NS::PcpArcTypePayload:
			case PXR_NS::PcpArcTypeInherit:
			case PXR_NS::PcpArcTypeSpecialize:
				// For these types we set a special comp arc badge. But we
				// keep looking for a variant.
				icon.badgeIcon = "out_USD_CompArcBadge.png";
				icon.pos = Ufe::UIInfoHandler::LowerRight;
				break;

			case PXR_NS::PcpArcTypeVariant :
				// Once we've found a variant type, use that badge and then
				// we are finished.
				icon.badgeIcon = "out_USD_CompArcBadgeV.png";
				icon.pos = Ufe::UIInfoHandler::LowerRight;
				keepLooking = false;
				break;
			default:
				break;
			}
			if (!keepLooking) break;
		}
	}

	return icon;
#else
	// No specific node type icon was found.
	return "";
#endif
}

#if UFE_PREVIEW_VERSION_NUM >= 2023
std::string UsdUIInfoHandler::treeViewTooltip(const Ufe::SceneItem::Ptr& item) const
{
	std::string tooltip;

	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
	if (usdItem)
	{
		// Loop thru all the composition arcs for the prim of the input scene item
		// and store the number of each arc type.
		std::multiset<PXR_NS::PcpArcType> arcTypeCount;
		PXR_NS::UsdPrimCompositionQuery query(usdItem->prim());
		for (const auto& arc : query.GetCompositionArcs())
		{
			auto arcType = arc.GetArcType();
			switch (arcType)
			{
				// We recognize these arc types.
			case PXR_NS::PcpArcTypeReference:
			case PXR_NS::PcpArcTypePayload:
			case PXR_NS::PcpArcTypeInherit:
			case PXR_NS::PcpArcTypeSpecialize:
			case PXR_NS::PcpArcTypeVariant :
				arcTypeCount.emplace(arcType);
				break;
			default:
				break;
			}
		}

		// Map of arc type with singular and plural string to display.
		static const std::map<PXR_NS::PcpArcType, std::pair<std::string, std::string>> arcTypeStrings{
			{ PXR_NS::PcpArcTypeReference, { "Reference", "References" } },
			{ PXR_NS::PcpArcTypePayload, { "Payload", "Payloads" } },
			{ PXR_NS::PcpArcTypeInherit, { "Inherit", "Inherits" } },
			{ PXR_NS::PcpArcTypeSpecialize, { "Specialize", "Specializes" } },
			{ PXR_NS::PcpArcTypeVariant, { "Variant", "Variants" } }
		};
		if (!arcTypeCount.empty())
		{
			tooltip += "<b>Composition Arcs:</b> ";
			bool needComma = false;
			for ( const auto& arcType : arcTypeStrings )
			{
				auto nb = arcTypeCount.count(arcType.first);
				if (nb >= 1)
				{
					if (needComma)
						tooltip += ", ";
					if (nb == 1) {
						tooltip += arcType.second.first;
					}
					else {
						tooltip += std::to_string(nb);
						tooltip += " ";
						tooltip += arcType.second.second;
					}
					needComma = true;
				}
			}
		}
	}
	return tooltip;
}
#endif

std::string UsdUIInfoHandler::getLongRunTimeLabel() const
{
	return "Universal Scene Description";
}


} // namespace ufe
} // namespace MayaUsd
