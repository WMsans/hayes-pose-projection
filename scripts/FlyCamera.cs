using Godot;

// Free-look camera. The keyboard is deliberately limited to movement:
// everything else in the tool is a UI control.
public partial class FlyCamera : Camera3D
{
	private const float Speed = 3.0f;
	private const float FastSpeed = 9.0f;
	private const float MouseSensitivity = 0.003f;
	private const float PitchLimit = 1.5f;

	private float _yaw;
	private float _pitch;

	public override void _Ready()
	{
		Vector3 angles = Rotation;
		_yaw = angles.Y;
		_pitch = angles.X;
	}

	public override void _UnhandledInput(InputEvent evt)
	{
		InputEventMouseMotion motion = evt as InputEventMouseMotion;
		if (motion != null && Input.MouseMode == Input.MouseModeEnum.Captured)
		{
			_yaw -= motion.Relative.X * MouseSensitivity;
			_pitch -= motion.Relative.Y * MouseSensitivity;
			_pitch = Mathf.Clamp(_pitch, -PitchLimit, PitchLimit);
			Rotation = new Vector3(_pitch, _yaw, 0.0f);
		}

		InputEventMouseButton button = evt as InputEventMouseButton;
		if (button != null && button.Pressed && Current)
		{
			Input.MouseMode = Input.MouseModeEnum.Captured;
		}

		if (evt.IsActionPressed("ui_cancel"))
		{
			Input.MouseMode = Input.MouseModeEnum.Visible;
		}
	}

	public override void _Process(double delta)
	{
		if (!Current)
		{
			return;
		}

		Vector3 move = Vector3.Zero;
		if (Input.IsPhysicalKeyPressed(Key.W)) { move -= Transform.Basis.Z; }
		if (Input.IsPhysicalKeyPressed(Key.S)) { move += Transform.Basis.Z; }
		if (Input.IsPhysicalKeyPressed(Key.A)) { move -= Transform.Basis.X; }
		if (Input.IsPhysicalKeyPressed(Key.D)) { move += Transform.Basis.X; }
		if (Input.IsPhysicalKeyPressed(Key.E)) { move += Vector3.Up; }
		if (Input.IsPhysicalKeyPressed(Key.Q)) { move -= Vector3.Up; }

		if (move.Length() > 0.0f)
		{
			float speed = Speed;
			if (Input.IsPhysicalKeyPressed(Key.Shift))
			{
				speed = FastSpeed;
			}
			Position += move.Normalized() * speed * (float)delta;
		}
	}

	public void SnapTo(Camera3D other)
	{
		GlobalTransform = other.GlobalTransform;
		Fov = other.Fov;
		KeepAspect = other.KeepAspect;

		Vector3 angles = GlobalTransform.Basis.GetEuler();
		_pitch = angles.X;
		_yaw = angles.Y;
		Rotation = new Vector3(_pitch, _yaw, 0.0f);
	}
}
