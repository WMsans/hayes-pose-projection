using Godot;

public partial class Main : Node
{
	public const float ImageSize = 1000.0f;

	public PoseData Data;
	public PoseFigure Figure;
	public Camera3D ChallengeCam;

	public override void _Ready()
	{
		Data = PoseLoader.LoadFromResources();

		Figure = new PoseFigure();
		Figure.Name = "PoseFigure";
		AddChild(Figure);
		Figure.Build();

		ChallengeCam = new Camera3D();
		ChallengeCam.Name = "ChallengeCam";
		ChallengeCam.KeepAspect = Camera3D.KeepAspectEnum.Height;
		// The pinhole model has the principal point at the image centre, so
		// half the image height subtends atan(500 / focal).
		ChallengeCam.Fov = Mathf.RadToDeg(2.0f * Mathf.Atan(ImageSize * 0.5f / Data.Focal));
		ChallengeCam.Near = 0.05f;
		AddChild(ChallengeCam);

		Figure.ShowFrame(Data.Frames[0]);
		AimChallengeCamera(Data.Frames[0]);

		string[] args = OS.GetCmdlineUserArgs();
		for (int i = 0; i < args.Length; i++)
		{
			if (args[i] == "--self-test")
			{
				GetTree().Quit(SelfTest.Run(Data, ChallengeCam, Figure));
				return;
			}
		}

		GD.Print("pose projection: " + Data.Frames.Length + " frames, focal " + Data.Focal);
	}

	// Challenge part (a): aim at the subject with no roll.
	public void AimChallengeCamera(PoseFrame frame)
	{
		ChallengeCam.Position = frame.CameraGodot;
		ChallengeCam.LookAt(frame.CentroidGodot, Vector3.Up);
	}
}
