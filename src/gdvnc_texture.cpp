#include "gdvnc_texture.h"

using namespace godot;

#define RFBTAG (void *)0x123456712345678

void GDVNC_Texture::_register_methods() {
	register_method((char *)"set_target_fps", &GDVNC_Texture::set_target_fps);
	register_method((char *)"get_target_fps", &GDVNC_Texture::get_target_fps);
	register_method((char *)"connect", &GDVNC_Texture::connect);
	register_method((char *)"disconnect", &GDVNC_Texture::disconnect);
	register_method((char *)"get_screen_size", &GDVNC_Texture::get_screen_size);
	register_method((char *)"update", &GDVNC_Texture::update);
	register_method((char *)"updateMouseState", &GDVNC_Texture::updateMouseState);
	register_method((char *)"updateKeyState", &GDVNC_Texture::updateKeyState);
}

// rfb calls this whenever there is a resize of our screen. 
rfbBool GDVNC_Texture::gdvnc_rfb_resize(rfbClient *p_client) {
	printf("RFB resize\n");

	GDVNC_Texture *texture = (GDVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	if (texture->framebuffer_allocated) {
		// free our old
		api->godot_free(p_client->frameBuffer);
		texture->framebuffer_allocated = false;
	}

	// we store our new size
	texture->width = p_client->width;
	texture->height = p_client->height;
	texture->bits_per_pixel = p_client->format.bitsPerPixel;

	printf("Resize received %i, %i (%i)\n", texture->width, texture->height, texture->bits_per_pixel);

	// and we allocate a new buffer
	p_client->frameBuffer = (uint8_t *)api->godot_alloc(texture->width * texture->height * 4);
	texture->framebuffer_allocated = (p_client->frameBuffer != NULL);

	// updating our texture happens after we receive updats
	texture->time_passed_since_update = 0.0;
	texture->framebuffer_is_dirty = true;

	return TRUE;
}

// rfb calls this whenever a portion of the screen has to be updated
void GDVNC_Texture::gdvnc_rfb_update (rfbClient *p_client, int p_x, int p_y, int p_w, int p_h) {
	printf("RFB update\n");

	GDVNC_Texture *texture = (GDVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	// all we do is set a flag that our framebuffer is dirty.
	texture->framebuffer_is_dirty = true;

	// updating our texture happens later
}

// rfb calls this when data was copied onto the clipboard on the server
void GDVNC_Texture::gdvnc_rfb_got_cut_text(rfbClient *p_client, const char *p_text, int p_textlen) {
	GDVNC_Texture *texture = (GDVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	// ignore for now but may copy this into our user data
}

// rfb calls this when our keyboard leds need to be turned on/off (numlock/caps/etc)
void GDVNC_Texture::gdvnc_rfb_kbd_leds(rfbClient *p_client, int p_value, int p_pad) {
	GDVNC_Texture *texture = (GDVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	// ignore this
}

// rfb calls this when we have an action associated with our text chat
void GDVNC_Texture::gdvnc_rfb_text_chat(rfbClient *p_client, int p_value, char *p_text) {
	GDVNC_Texture *texture = (GDVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	// ignore this
}

// rfb calls this when the server requests a password
char *GDVNC_Texture::gdvnc_rfb_get_password(rfbClient *p_client) {
	char *pwd = NULL;

	GDVNC_Texture *texture = (GDVNC_Texture *)rfbClientGetClientData(p_client, RFBTAG);

	// libvnc is a bit too security concious. It will clear the memory and then free it
	// so yes.. we malloc, they free....
	pwd = (char *) calloc(1, strlen(texture->password)+1);
	strcpy(pwd, texture->password);

	return pwd;
}

GDVNC_Texture::GDVNC_Texture() {
	rfbCl = NULL;
	width = 0;
	height = 0;
	is_connected = false;
	framebuffer_allocated = false;
	target_fps = 15.0;
	strcpy(password, "password");
}

GDVNC_Texture::~GDVNC_Texture() {
	printf("Cleanup gdvnc\n");

	if (framebuffer_allocated) {
		// free our old
		api->godot_free(rfbCl->frameBuffer);
		framebuffer_allocated = false;
	}

	if (rfbCl != NULL) {
		rfbClientCleanup(rfbCl);
		rfbCl = NULL;
	}
}

void GDVNC_Texture::set_target_fps(float p_fps) {
	if (p_fps < 0)
		return;

	target_fps = p_fps;
}

float GDVNC_Texture::get_target_fps() {
	return target_fps;
}

bool GDVNC_Texture::connect(String p_host, String p_password) {
	char program[] = "godot";
	char host[256];

	// if we're already connected, lets disconnect
	disconnect();

	CharString hostAsUTF8 = p_host.utf8();
	strcpy(host, hostAsUTF8.get_data());

	CharString passwordAsUTF8 = p_password.utf8();
	strcpy(password, passwordAsUTF8.get_data());

	printf("Attempting connect to %s\n", host);

	// create our client
	rfbCl = rfbGetClient(8, 3, 4);

	// set some callback pointers
	rfbCl->MallocFrameBuffer = GDVNC_Texture::gdvnc_rfb_resize;
	rfbCl->canHandleNewFBSize = TRUE;
	rfbCl->GotFrameBufferUpdate = GDVNC_Texture::gdvnc_rfb_update;
	rfbCl->GotXCutText = GDVNC_Texture::gdvnc_rfb_got_cut_text;
	rfbCl->HandleKeyboardLedState = GDVNC_Texture::gdvnc_rfb_kbd_leds;
	rfbCl->HandleTextChat = GDVNC_Texture::gdvnc_rfb_text_chat;
	rfbCl->GetPassword = GDVNC_Texture::gdvnc_rfb_get_password;

	// this lets rfb know that we want a pointer to our instance when it calls us
	rfbClientSetClientData(rfbCl, RFBTAG, this);

	// initialize our client
	int argc = 2;
	char *argv[2];
	argv[0] = program;
	argv[1] = host;
	is_connected = rfbInitClient(rfbCl, &argc, argv);

	if (is_connected) {
		printf("Success\n");

		return true;
	} else {
		printf("Failure\n");

		// cleanup...
		disconnect();
		return false;
	}
}

void GDVNC_Texture::disconnect() {
	if (rfbCl != NULL) {
		rfbClientCleanup(rfbCl);
		rfbCl = NULL;
	}
	is_connected = false;
}

Vector2 GDVNC_Texture::get_screen_size() {
	Vector2 ret;
	ret.x=width;
	ret.y=height;
	return ret;
}

bool GDVNC_Texture::update(float p_delta) {
	if (is_connected && (rfbCl!=NULL)) {
		// we want to be none blocking, even 1ms might be too long a timeout
		// should also check for negative result of waitformessage
		int cnt = WaitForMessage(rfbCl, 1);
		if (cnt < 0) {
			// need to log error and possibly disconnect if we've lost connection
			return false;
		} else if (cnt > 0) {
			HandleRFBServerMessage(rfbCl);
		}

		if (framebuffer_allocated && framebuffer_is_dirty) {
			// updates can update only small parts of the window, we build in a delay before we update our texture
			time_passed_since_update += p_delta;
			if (time_passed_since_update > (1.0 / target_fps)) {
				// update screen
				printf("Update\n");

				// first copy our buffer into a PoolByteArray
				PoolByteArray image_data;
				image_data.resize(width * height * 4);
				{
					PoolByteArray::Write image_write = image_data.write();
					memcpy(image_write.ptr(), rfbCl->frameBuffer, width * height * 4);
				}

				// create an image from this
				Ref<Image> img;
				img.instance();
				img->create_from_data(width, height, false, 5, image_data); // 5 = FORMAT_RGBA8

				owner->create_from_image(img, 5);
				img.unref();

				framebuffer_is_dirty = false;
				time_passed_since_update = 0.0;
			}
		}

		return true;
	} else {
		// need to log error
		return false;
	}
}

bool GDVNC_Texture::updateMouseState(int p_x, int p_y, int p_mask) {
	if (is_connected && (rfbCl!=NULL)) {
		int mask;

		// middle and right buttons are reversed
		if ((p_mask & 1) == 1) {
			mask += 1;
		}

		if ((p_mask & 2) == 2) {
			mask += 4;
		}

		if ((p_mask & 4) == 4) {
			mask += 2;
		}

		// add the other mouse buttons as is...
		mask += (p_mask & 0xFFF8);

		if (SendPointerEvent(rfbCl, p_x, p_y, mask)) {
			return true;
		} else {
			// need to log error and possibly disconnect if we've lost connection
			return false;
		}
	} else {
		// need to log error
		return false;
	}
}

bool GDVNC_Texture::updateKeyState(int p_scancode, bool p_is_down) {
	if (is_connected && (rfbCl!=NULL)) {
		// VNC requires certain codes to be different
		switch (p_scancode) {
			case GlobalConstants::KEY_ENTER:
				p_scancode = XK_Return;
				break;
			case GlobalConstants::KEY_META:
				p_scancode = XK_Meta_L;
				break;
			case GlobalConstants::KEY_BACKSPACE:
				p_scancode = XK_BackSpace; /* back space, back char */
				break;
			case GlobalConstants::KEY_TAB:
				p_scancode = XK_Tab;
				break;
//			case GlobalConstants::KEY_LF:
//				p_scancode = XK_Linefeed; /* Linefeed, LF */
//				break;
			case GlobalConstants::KEY_CLEAR:
				p_scancode = XK_Clear;
				break;
			case GlobalConstants::KEY_PAUSE:
				p_scancode = XK_Pause; /* Pause, hold */
				break;
			case GlobalConstants::KEY_SCROLLLOCK:
				p_scancode = XK_Scroll_Lock;
				break;
//			case GlobalConstants::KEY_SEQ_REQ:
//				p_scancode = XK_Sys_Req;
//				break;
			case GlobalConstants::KEY_ESCAPE:
				p_scancode = XK_Escape;
				break;
			case GlobalConstants::KEY_DELETE:
				p_scancode = XK_Delete; /* Delete, rubout */
				break;

			case GlobalConstants::KEY_HOME:
				p_scancode = XK_Home;
				break;
			case GlobalConstants::KEY_LEFT:
				p_scancode = XK_Left; /* Move left, left arrow */
				break;
			case GlobalConstants::KEY_UP:
				p_scancode = XK_Up; /* Move up, up arrow */
				break;
			case GlobalConstants::KEY_RIGHT:
				p_scancode = XK_Right; /* Move right, right arrow */
				break;
			case GlobalConstants::KEY_DOWN:
				p_scancode = XK_Down; /* Move down, down arrow */
				break;
//			case GlobalConstants::KEY_PRIOR:
//				p_scancode = XK_Prior; /* Prior, previous */
//				break;
			case GlobalConstants::KEY_PAGEUP:
				p_scancode = XK_Page_Up;
				break;
//			case GlobalConstants::KEY_NEXT:
//				p_scancode = XK_Next; /* Next */
//				break;
			case GlobalConstants::KEY_PAGEDOWN:
				p_scancode = XK_Page_Down;
				break;
			case GlobalConstants::KEY_END:
				p_scancode = XK_End; /* EOL */
				break;
//			case GlobalConstants::KEY_BEGIN:
//				p_scancode = XK_Begin; /* BOL */
//				break;


			case GlobalConstants::KEY_SHIFT:
				p_scancode = XK_Shift_L; /* Left shift */
				break;
//			case GlobalConstants::KEY_SHIFT:
//				p_scancode = XK_Shift_R; /* Right shift */
//				break;
			case GlobalConstants::KEY_CONTROL:
				p_scancode = XK_Control_L; /* Left control */
				break;
//			case GlobalConstants::KEY_CONTROL:
//				p_scancode = XK_Control_R; /* Right control */
//				break;
			case GlobalConstants::KEY_CAPSLOCK:
				p_scancode = XK_Caps_Lock; /* Caps lock */
				break;
//			case GlobalConstants::KEY_SHIFTLOCK:
//				p_scancode = XK_Shift_Lock; /* Shift lock */
//				break;

//			case GlobalConstants::KEY_META_L:
//				p_scancode = XK_Meta_L; /* Left meta */
//				break;
//			case GlobalConstants::KEY_META_R:
//				p_scancode = XK_Meta_R; /* Right meta */
//				break;
			case GlobalConstants::KEY_ALT:
				p_scancode = XK_Alt_L; /* Left alt */
				break;
//			case GlobalConstants::KEY_ALT:
//				p_scancode = XK_Alt_R; /* Right alt */
//				break;

			default:
				// use as is
				break;
		}


		if (SendKeyEvent(rfbCl, p_scancode, p_is_down)) {
			return true;
		} else {
			// need to log error and possibly disconnect if we've lost connection
			return false;
		}
	} else {
		// need to log error
		return false;
	}
}

