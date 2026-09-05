using Godot;

// Textbook pinhole projection, done by hand from the raw millimetre data.
// It never touches the scene graph or a Camera3D, so it is a genuinely
// independent check on the Godot implementation.
public class ManualProjector : IProjector
{
	// The frames are 1000x1000, so the principal point is the centre.
	public const float PrincipalPoint = 500.0f;

	private PoseFrame _frame;
	private float _focal;
	private Vector3 _forward;
	private Vector3 _right;
	private Vector3 _up;

	public void Begin(PoseFrame frame, float focal)
	{
		_frame = frame;
		_focal = focal;

		// Challenge part (a): point the camera at the subject. The centroid
		// of the joints is used rather than the hip so crouched and reaching
		// poses stay framed.
		_forward = (frame.CentroidWorld - frame.CameraWorld).Normalized();

		// World up is +Z here, not +Y. Crossing it out of forward gives a
		// camera with no roll.
		Vector3 worldUp = new Vector3(0.0f, 0.0f, 1.0f);
		_right = _forward.Cross(worldUp).Normalized();
		_up = _right.Cross(_forward);
	}

	public Vector2 Project(int jointIndex)
	{
		Vector3 d = _frame.JointsWorld[jointIndex] - _frame.CameraWorld;
		float x = d.Dot(_right);
		float y = d.Dot(_up);
		float z = d.Dot(_forward);

		float u = _focal * x / z + PrincipalPoint;
		// Image rows count downward while _up points up, so v is subtracted.
		float v = PrincipalPoint - _focal * y / z;
		return new Vector2(Mathf.Round(u * 1000.0f) / 1000.0f, Mathf.Round(v * 1000.0f) / 1000.0f);
	}
}
