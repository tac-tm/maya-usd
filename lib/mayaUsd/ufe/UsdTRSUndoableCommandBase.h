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
#pragma once

#include "../base/api.h"
#include "UsdSceneItem.h"

#include <ufe/transform3dUndoableCommands.h>
#include <ufe/observer.h>

#include <pxr/usd/usd/attribute.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Base class for translate, rotate, scale undoable commands.
//
// UsdRotateUndoableCommand causes USD assertion when used with GfVec3d:
//
// Coding Error: in _SetValueImpl at line 5619 of [...]/usd/pxr/usd/usd/stage.cpp -- Type mismatch for </pCylinder1/pCube1.xformOp:rotateXYZ>: expected 'GfVec3f', got 'GfVec3d'
//
// Therefore, this class is templated on GfVec3f or GfVec3d, so rotate can use
// GfVec3f.  To be investigated.  PPT, 9-Apr-2020.
//
template<class V>
class MAYAUSD_CORE_PUBLIC UsdTRSUndoableCommandBase : public Ufe::Observer,
        public std::enable_shared_from_this<UsdTRSUndoableCommandBase<V> >
{
protected:

    UsdTRSUndoableCommandBase(
        const UsdSceneItem::Ptr& item, double x, double y, double z);
    ~UsdTRSUndoableCommandBase() = default;

    // Initialize the command.
    void initialize();

    // Undo and redo implementations.
    void undoImp();
    void redoImp();

    // Set the new value of the command (for redo), and execute the command.
    void perform(double x, double y, double z);

    // UFE item (and its USD prim) may change after creation time (e.g.
    // parenting change caused by undo / redo of other commands in the undo
    // stack), so always return current data.
    inline UsdPrim prim() const { return fItem->prim(); }
    inline Ufe::Path path() const { return fItem->path(); }

    // Hooks to be implemented by the derived class: name of the attribute set
    // by the command, implementation of perform(), and add empty attribute.
    // Implementation of cannotInit() in this class returns false.
    virtual TfToken attributeName() const = 0;
    virtual void performImp(double x, double y, double z) = 0;
    virtual void addEmptyAttribute() = 0;
    virtual bool cannotInit() const;

private:

    // Overridden from Ufe::Observer
    void operator()(const Ufe::Notification& notification) override;

    template<class N> void checkNotification(const N* notification);

    inline UsdAttribute attribute() const {
      return prim().GetAttribute(attributeName());
    }

    UsdSceneItem::Ptr fItem;
    V                 fPrevValue;
    V                 fNewValue;
    bool              fOpAdded{false};
    bool              fDoneOnce{false};
}; // UsdTRSUndoableCommandBase

} // namespace ufe
} // namespace MayaUsd
