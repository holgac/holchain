#include <X11/Xlib.h>
#include <X11/Xatom.h>

static Time
get_timestamp (void)
{
  XEvent event;
  XChangeProperty (display, window, XA_WM_NAME, XA_STRING, 8,
                   PropModeAppend, NULL, 0);
  while (1) {
    XNextEvent (display, &event);
    if (event.type == PropertyNotify)
      return event.xproperty.time;
  }
}

static void
set_selection (Display* display, Atom selection, unsigned char* sel)
{
  XEvent event;
  IncrTrack * it;
}

static IncrTrack *
find_incrtrack (Atom atom)
{
  IncrTrack * iti;
  for (iti = incrtrack_list; iti; iti = iti->next) {
    if (atom == iti->property) return iti;
  }
  return NULL;
}

static HandleResult
continue_incr (IncrTrack * it)
{
  HandleResult retval = HANDLE_OK;
  if (it->state == S_INCR_1) {
    retval = incr_stage_1 (it);
  } else if (it->state == S_INCR_2) {
    retval = incr_stage_2 (it);
  }
  /* If that completed the INCR, deal with completion */
  if ((retval & HANDLE_INCOMPLETE) == 0) {
    complete_incr (it, retval);
  }
  return retval;
}


int main(int argc, char** argv) {
  Display *display = XOpenDisplay(NULL);
  Window root = XDefaultRootWindow(display);
  int black = BlackPixel(display, DefaultScreen(display));
  Window window = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, black, black);
  XSetSelectionOwner (display, XA_PRIMARY, window, get_timestamp());
  /* XGetSelectionOwner does a round trip to the X server, so there is
   * no need to call XSync here. */
  owner = XGetSelectionOwner(display, XA_PRIMARY);
  if (owner != window) {
    throw "failed"; // TODO: fix
  }
  unsigned char* sel = (unsigned char*)argv[1];
  XEvent event;
  utf8_atom = XInternAtom (display, "UTF8_STRING", True);
  for (;;) {
    /* Flush before unblocking signals so we send replies before exiting */
    XFlush (display);
    XNextEvent (display, &event);

    switch (event.type) {
    case SelectionClear:
      printf("Got SelectionClear\n");
      if (event.xselectionclear.selection == XA_PRIMARY) return;
      break;
    case SelectionRequest:
      printf("Got SelectionRequest\n");
      if (event.xselectionrequest.selection != XA_PRIMARY) break;
      // if (!handle_selection_request (event, sel)) return;

      break;
    case PropertyNotify:
      printf("Got PropertyNotify\n");
      if (event.xproperty.state != PropertyDelete) break;

      it = find_incrtrack (event.xproperty.atom);

      if (it != NULL) {
        continue_incr (it);
      }

      break;
    default:
      break;
    }
  }
  return 0;
}
