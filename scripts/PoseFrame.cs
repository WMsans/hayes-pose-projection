using Godot;

// One row of poses.txt: a camera position and 14 joint positions.
// Kept in both the source coordinate system and Godot's, because the
// manual projector works from the raw values and the scene works from
// the converted ones.
public class PoseFrame
{
	public Vector3 CameraWorld;    // millimetres, Z-up
	public Vector3 CameraGodot;    // metres, Y-up
	public Vector3[] JointsWorld;  // 14 joints, millimetres, Z-up
	public Vector3[] JointsGodot;  // 14 joints, metres, Y-up
	public Vector3 CentroidWorld;
	public Vector3 CentroidGodot;

	// The source data is Z-up in millimetres; Godot is Y-up in metres.
	// Mapping (x, y, z) to (x, z, -y) keeps the system right-handed.
	public static Vector3 ToGodot(Vector3 world)
	{
		return new Vector3(world.X, world.Z, -world.Y) / 1000.0f;
	}

	public void ComputeDerived()
	{
		Vector3 sum = Vector3.Zero;
		for (int i = 0; i < JointsWorld.Length; i++)
		{
			sum += JointsWorld[i];
		}
		CentroidWorld = sum / JointsWorld.Length;
		CentroidGodot = ToGodot(CentroidWorld);

		CameraGodot = ToGodot(CameraWorld);
		JointsGodot = new Vector3[JointsWorld.Length];
		for (int i = 0; i < JointsWorld.Length; i++)
		{
			JointsGodot[i] = ToGodot(JointsWorld[i]);
		}
	}
}
