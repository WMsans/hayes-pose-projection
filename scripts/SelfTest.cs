using Godot;

// Runs headlessly with --self-test. Compares the two projectors over every
// joint of every frame. A disagreement means one of them is wrong, and the
// exported table cannot be trusted.
public static class SelfTest
{
	private const float Tolerance = 0.01f;

	public static int Run(PoseData data, Camera3D camera, PoseFigure figure)
	{
		int failures = 0;
		ManualProjector manual = new ManualProjector();
		GodotProjector godot = new GodotProjector(camera, figure);

		for (int f = 0; f < data.Frames.Length; f++)
		{
			PoseFrame frame = data.Frames[f];
			figure.ShowFrame(frame);
			camera.Position = frame.CameraGodot;
			camera.LookAt(frame.CentroidGodot, Vector3.Up);

			manual.Begin(frame, data.Focal);
			godot.Begin(frame, data.Focal);

			for (int j = 0; j < PoseData.JointCount; j++)
			{
				Vector2 a = manual.Project(j);
				Vector2 b = godot.Project(j);
				float dx = Mathf.Abs(a.X - b.X);
				float dy = Mathf.Abs(a.Y - b.Y);
				if (dx > Tolerance || dy > Tolerance)
				{
					GD.PrintErr("frame " + f + " joint " + j + ": manual " + a + " godot " + b);
					failures++;
				}

				if (a.X < 0.0f || a.X > 1000.0f || a.Y < 0.0f || a.Y > 1000.0f)
				{
					GD.PrintErr("frame " + f + " joint " + j + " is outside the image: " + a);
					failures++;
				}
			}
		}

		if (failures == 0)
		{
			GD.Print("self-test: OK, both projectors agree on all 280 points");
			return 0;
		}
		GD.PrintErr("self-test: " + failures + " failures");
		return 1;
	}
}
