extends Area

onready var vnc = preload("res://bin/gdvnc_texture.gdns").new()
onready var screen_node = get_node("Screen")
onready var collider_node = get_node("CollisionShape")

var prev_pos = null
var last_click_pos = null
var mouse_on_window = false

func _ready():
	# assign our VNC thingy as our texture
	var material = screen_node.get_surface_material(0)
	material.albedo_texture = vnc
	
	# our image is upside down, right to left so flip our UV
	material.uv1_scale = Vector3(-1.0, -1.0, 1.0)
	material.uv1_offset = Vector3(1.0, 1.0, 0.0)
	
	# this returns true on a successful connect
	vnc.connect("192.168.1.3", "test")
	vnc.set_target_fps(15.0)

func _process(delta):
	# work around to get around not having an update process...
	# this basically allows the plugin to process messages from the server and update the texture
	vnc.update(delta)

# The input functions below are based on the viewport/gui_in_3d example and show how we transform the mouse
# pointer to input on our VNC window.
# In VR have a look at my Sponza demo code where I do something similar but using the pointer function
# I added to the controller.

func _input(event):
	if (event is InputEventKey):
		# this needs to be improved, if we press a key with the mouse over our window
		# and then release it when we're outside of the window, it won't register
		if mouse_on_window:
			# note, most of these are correctly handled in our module I'm converting them, but if
			# I got one wrong the mapping will need to be edited
			vnc.updateKeyState(event.scancode, event.pressed)


func _on_VNCWindow_input_event( camera, event, click_pos, click_normal, shape_idx ):
	# Use click pos (click in 3d space, convert to area space)
	var pos = get_global_transform().affine_inverse()
	var half_screen_size = (screen_node.mesh.size / 2.0)
	var remote_screen_size = vnc.get_screen_size()
	
	# the click pos is not zero, then use it to convert from 3D space to area space
	if (click_pos.x != 0 or click_pos.y != 0 or click_pos.z != 0):
		pos *= click_pos
		last_click_pos = click_pos
	else:
		# Otherwise, we have a motion event and need to use our last click pos
		# and move it according to the relative position of the event.
		# NOTE: this is not an exact 1-1 conversion, but it's pretty close
		pos *= last_click_pos
		if (event is InputEventMouseMotion or event is InputEventScreenDrag):
			pos.x += half_screen_size.x * event.relative.x / remote_screen_size.x
			pos.y += half_screen_size.y * event.relative.y / remote_screen_size.y
			last_click_pos = pos
  
	# Convert to 2D
	pos = Vector2(pos.x, pos.y)
	
	#  Adjust based on our screen size to get a value between -1.0, -1.0 to 1.0, 1.0
	pos /= half_screen_size
  
	# Convert to viewport coordinate system
	# Convert pos to a range from (0 - 1)
	pos.y *= -1
	pos += Vector2(1, 1)
	pos = pos / 2
  
	# Convert pos to be in range of the viewport
	pos *= remote_screen_size
		
	# I think our button mask 
	vnc.updateMouseState(pos.x, pos.y, event.button_mask)


func _on_VNCWindow_mouse_entered():
	mouse_on_window = true

func _on_VNCWindow_mouse_exited():
	mouse_on_window = false
