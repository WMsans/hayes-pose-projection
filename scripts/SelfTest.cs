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
				Vector2 manualPoint = manual.Project(j);
				Vector2 godotPoint = godot.Project(j);
				if (!IsFinite(manualPoint) || !IsFinite(godotPoint))
				{
					GD.PrintErr("frame " + f + " joint " + j + " produced a non-finite point: manual " + manualPoint + " godot " + godotPoint);
					failures++;
					continue;
				}

				float dx = Mathf.Abs(manualPoint.X - godotPoint.X);
				float dy = Mathf.Abs(manualPoint.Y - godotPoint.Y);
				if (dx > Tolerance || dy > Tolerance)
				{
					GD.PrintErr("frame " + f + " joint " + j + ": manual " + manualPoint + " godot " + godotPoint);
					failures++;
				}

				if (!IsInsideImage(manualPoint))
				{
					GD.PrintErr("frame " + f + " joint " + j + " manual point is outside the image: " + manualPoint);
					failures++;
				}
				if (!IsInsideImage(godotPoint))
				{
					GD.PrintErr("frame " + f + " joint " + j + " Godot point is outside the image: " + godotPoint);
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

	private static bool IsFinite(Vector2 point)
	{
		return !float.IsNaN(point.X) && !float.IsInfinity(point.X)
			&& !float.IsNaN(point.Y) && !float.IsInfinity(point.Y);
	}

	private static bool IsInsideImage(Vector2 point)
	{
		return point.X >= 0.0f && point.X <= 1000.0f
			&& point.Y >= 0.0f && point.Y <= 1000.0f;
	}
}
