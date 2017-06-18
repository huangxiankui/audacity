/**********************************************************************

Audacity: A Digital Audio Editor

UIHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_UI_HANDLE__
#define __AUDACITY_UI_HANDLE__

#include <utility>

class wxDC;
class wxRect;
class wxRegion;
class wxWindow;

class AudacityProject;
struct HitTestPreview;
class TrackPanelCell;
struct TrackPanelMouseEvent;
struct TrackPanelMouseState;

#include "MemoryX.h"

// A TrackPanelCell reports a handle object of some subclass, in response to a
// hit test at a mouse position; then this handle processes certain events,
// and maintains necessary state through click-drag-release event sequences.
class UIHandle /* not final */
{
public:
   // See RefreshCode.h for bit flags:
   using Result = unsigned;

   // Future: may generalize away from current Track class
   using Cell = TrackPanelCell;

   // Argument for the drawing function
   enum DrawingPass {
      Cells,
      Panel,
   };

   virtual ~UIHandle() = 0;

   // Assume hit test (implemented in other classes) was positive.
   // May return Cancelled, overriding the hit test decision and stopping drag.
   // Otherwise the framework will later call Release or Cancel after
   // some number of Drag calls.
   virtual Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject) = 0;

   // Assume previously Clicked and not yet Released or Cancelled.
   // pCell may be other than for Click; may be NULL, and rect empty.
   // Return value may include the Cancelled return flag,
   // in which case the handle will not be invoked again.
   virtual Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject) = 0;

   // Can be called when the handle has been hit but not yet clicked,
   // or called after Drag().
   // Specifies cursor and status bar message.
   virtual HitTestPreview Preview
      (const TrackPanelMouseState &state, const AudacityProject *pProject) = 0;

   // Assume previously Clicked and not yet Released or Cancelled.
   // event.pCell may be other than for Click; may be NULL, and rect empty.
   // Can use pParent as parent to pop up a context menu,
   // connecting and disconnecting event handlers for the menu items.
   // Cancelled in return flags has no effect.
   virtual Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
       wxWindow *pParent) = 0;

   // Assume previously Clicked and not yet Released or Cancelled.
   // Cancelled in return flags has no effect.
   virtual Result Cancel(AudacityProject *pProject) = 0;

   // Draw extras over cells.  Default does nothing.
   // Supplies only the whole panel rectangle for now.
   // If pass is Cells, then any drawing that extends outside the cells
   // is later overlaid with the cell bevels and the empty background color.
   // Otherwise (Panel), it is a later drawing pass that will not be overlaid.
   virtual void DrawExtras
      (DrawingPass pass,
       wxDC * dc, const wxRegion &updateRegion, const wxRect &panelRect);

   // Whether to force Release (not Cancel!) of the drag when a
   // keystroke command is about to be dispatched.  Default is always false.
   // When default is false, any remembered pointers to tracks should be
   // weak_ptrs.
   virtual bool StopsOnKeystroke();

   // Notification after a command is dispatched; generally, it will need to
   // be overridden only in case StopsOnKeystroke() returns false.  Default
   // does nothing.
   // PRL: all former uses of this are now accomplished with weak_ptr instead
   // to avoid dangling pointers to tracks.  But maybe there will be a future
   // use?
   virtual void OnProjectChange(AudacityProject *pProject);

public:
   Result GetChangeHighlight() const { return mChangeHighlight; }
   void SetChangeHighlight(Result val) { mChangeHighlight = val; }

   // If AssignUIHandlePtr is used, then this function is also called before any
   // overwrite.
   // Make overloads of this for other subclasses, to cause refresh
   // of the cell during mouse motion within it.
   static UIHandle::Result NeedChangeHighlight
   (const UIHandle &/*oldState*/, const UIHandle &/*newState*/)
   {
      return 0;
   }

protected:
   // Derived classes can set this nonzero in a constructor, which is enough
   // to cause repaint of the cell whenever the pointer hits the target,
   // or leaves it without clicking, or releases or escapes from a drag.
   Result mChangeHighlight { 0 };

};

using UIHandlePtr = std::shared_ptr<UIHandle>;

// A frequent convenience for defining a hit test.
// Construct a NEW handle as if hit the first time; then either keep it, or
// use it to overwrite the state of a previously constructed handle that has not
// yet been released.
template<typename Subclass>
std::shared_ptr<Subclass> AssignUIHandlePtr
( std::weak_ptr<Subclass> &holder, const std::shared_ptr<Subclass> &pNew )
{
   // Either assign to a null weak_ptr, or else rewrite what the weak_ptr
   // points at.  Thus a handle already pointed at changes its state but not its
   // identity.  This may matter for the framework that holds the strong
   // pointers.
   auto ptr = holder.lock();
   if (!ptr) {
      holder = pNew;
      return pNew;
   }
   else {
      auto code = Subclass::NeedChangeHighlight( *ptr, *pNew );
      *ptr = std::move(*pNew);
      ptr->SetChangeHighlight( code );
      return ptr;
   }
}

#endif
