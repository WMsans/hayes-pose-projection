using Godot;

public partial class Main : Node
{
	public const float ImageSize = 1000.0f;

	public PoseData Data;
	public PoseFigure Figure;
	public Camera3D ChallengeCam;
	public FlyCamera Fly;
	public ProjectionOverlay Overlay;
	public ControlPanel Panel;
	public bool TwoDimensionalView;
	public ProjectionMethod Method = ProjectionMethod.ManualPinhole;
	public int FrameIndex;

	[Export] public ProjectionMethod DefaultMethod = ProjectionMethod.ManualPinhole;

	private MeshInstance3D _cameraMarker;
	private MeshInstance3D _cameraAimLine;
	private ImmediateMesh _cameraAimLineMesh;
	private StandardMaterial3D _cameraAimLineMaterial;
	private CanvasLayer _canvas;
	private ManualProjector _manual = new ManualProjector();
	private GodotProjector _godot;

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

		BuildEnvironment();

		Fly = new FlyCamera();
		Fly.Name = "FlyCamera";
		AddChild(Fly);

		Method = DefaultMethod;
		_godot = new GodotProjector(ChallengeCam, Figure);

		_canvas = new CanvasLayer();
		_canvas.Name = "Canvas";
		AddChild(_canvas);

		Overlay = new ProjectionOverlay();
		Overlay.Name = "Overlay";
		Overlay.SetAnchorsPreset(Control.LayoutPreset.FullRect);
		Overlay.MouseFilter = Control.MouseFilterEnum.Ignore;
		Overlay.Visible = false;
		_canvas.AddChild(Overlay);

		Panel = new ControlPanel();
		Panel.Name = "ControlPanel";
		_canvas.AddChild(Panel);

		ShowFrame(0);

		Fly.SnapTo(ChallengeCam);
		Fly.Current = true;

		Panel.Build(this);

		string[] args = OS.GetCmdlineUserArgs();
		for (int i = 0; i < args.Length; i++)
		{
			if (args[i] == "--self-test")
			{
				GetTree().Quit(SelfTest.Run(Data, ChallengeCam, Figure));
				return;
			}

			if (args[i] == "--export-csv")
			{
				Exporter exporter = new Exporter(this);
				exporter.WriteTables();
				GD.Print("wrote tables to " + Exporter.OutputDirectory());
				GetTree().Quit(0);
				return;
			}
		}

		GD.Print("pose projection: " + Data.Frames.Length + " frames, focal " + Data.Focal);
	}

	public async System.Threading.Tasks.Task<string> RunExport()
	{
		Exporter exporter = new Exporter(this);
		exporter.WriteTables();
		await exporter.WriteImages();
		Panel.UpdateReadouts();
		return "wrote tables and 40 images to " + Exporter.OutputDirectory();
	}

	public void ShowFrame(int index)
	{
		FrameIndex = Mathf.Clamp(index, 0, Data.Frames.Length - 1);
		PoseFrame frame = Data.Frames[FrameIndex];
		Figure.ShowFrame(frame);
		AimChallengeCamera(frame);

		if (Overlay != null)
		{
			// CSV/self-test runs do not render photographs, and headless Godot
			// does not provide the PNG loader needed by the GUI overlay.
			if (DisplayServer.GetName() != "headless")
			{
				Overlay.Photograph = LoadFrameTexture(FrameIndex);
			}
			Overlay.Refresh(ProjectCurrent(Method));
		}
	}

	public void SetProjectionMethod(ProjectionMethod method)
	{
		Method = method;
		if (TwoDimensionalView && Overlay != null)
		{
			Overlay.Refresh(ProjectCurrent(Method));
		}
	}

	public Vector2[] ProjectCurrent(ProjectionMethod method)
	{
		PoseFrame frame = Data.Frames[FrameIndex];
		IProjector projector = _manual;
		if (method == ProjectionMethod.GodotUnproject)
		{
			projector = _godot;
		}
		projector.Begin(frame, Data.Focal);

		Vector2[] points = new Vector2[PoseData.JointCount];
		for (int j = 0; j < points.Length; j++)
		{
			points[j] = projector.Project(j);
		}
		return points;
	}

	public void SetTwoDimensionalView(bool on)
	{
		TwoDimensionalView = on;
		Overlay.Visible = on;
		ChallengeCam.Current = on;
		Fly.Current = !on;
		if (on)
		{
			Input.MouseMode = Input.MouseModeEnum.Visible;
		}
		Overlay.Refresh(ProjectCurrent(Method));
	}

	public static Texture2D LoadFrameTexture(int index)
	{
		string path = "res://data/frames/" + index.ToString("00") + ".png";
		return GD.Load<Texture2D>(path);
	}

	// Challenge part (a): aim at the subject with no roll.
	public void AimChallengeCamera(PoseFrame frame)
	{
		ChallengeCam.Position = frame.CameraGodot;
		ChallengeCam.LookAt(frame.CentroidGodot, Vector3.Up);
		if (_cameraMarker != null)
		{
			_cameraMarker.Position = frame.CameraGodot;
		}
		if (_cameraAimLineMesh != null)
		{
			_cameraAimLineMesh.ClearSurfaces();
			_cameraAimLineMesh.SurfaceBegin(Mesh.PrimitiveType.Lines, _cameraAimLineMaterial);
			_cameraAimLineMesh.SurfaceAddVertex(frame.CameraGodot);
			_cameraAimLineMesh.SurfaceAddVertex(frame.CentroidGodot);
			_cameraAimLineMesh.SurfaceEnd();
		}
	}

	// A floor grid and a marker at the challenge camera, so the geometry of
	// part (a) is visible rather than just asserted.
	public void BuildEnvironment()
	{
		ImmediateMesh grid = new ImmediateMesh();
		StandardMaterial3D lineMaterial = new StandardMaterial3D();
		lineMaterial.AlbedoColor = new Color(0.75f, 0.75f, 0.78f);
		lineMaterial.ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded;
		lineMaterial.VertexColorUseAsAlbedo = true;

		grid.SurfaceBegin(Mesh.PrimitiveType.Lines, lineMaterial);
		for (int i = -10; i <= 10; i++)
		{
			float at = i * 0.5f;
			grid.SurfaceSetColor(new Color(0.75f, 0.75f, 0.78f));
			grid.SurfaceAddVertex(new Vector3(at, 0.0f, -5.0f));
			grid.SurfaceAddVertex(new Vector3(at, 0.0f, 5.0f));
			grid.SurfaceAddVertex(new Vector3(-5.0f, 0.0f, at));
			grid.SurfaceAddVertex(new Vector3(5.0f, 0.0f, at));
		}
		grid.SurfaceEnd();

		MeshInstance3D floor = new MeshInstance3D();
		floor.Name = "FloorGrid";
		floor.Mesh = grid;
		AddChild(floor);

		BoxMesh box = new BoxMesh();
		box.Size = new Vector3(0.14f, 0.14f, 0.14f);
		StandardMaterial3D markerMaterial = new StandardMaterial3D();
		markerMaterial.AlbedoColor = new Color(0.95f, 0.65f, 0.10f);
		markerMaterial.ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded;

		_cameraMarker = new MeshInstance3D();
		_cameraMarker.Name = "CameraMarker";
		_cameraMarker.Mesh = box;
		_cameraMarker.MaterialOverride = markerMaterial;
		AddChild(_cameraMarker);

		_cameraAimLineMesh = new ImmediateMesh();
		_cameraAimLineMaterial = new StandardMaterial3D();
		_cameraAimLineMaterial.AlbedoColor = new Color(0.95f, 0.65f, 0.10f);
		_cameraAimLineMaterial.ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded;
		_cameraAimLine = new MeshInstance3D();
		_cameraAimLine.Name = "CameraAimLine";
		_cameraAimLine.Mesh = _cameraAimLineMesh;
		_cameraAimLine.MaterialOverride = _cameraAimLineMaterial;
		AddChild(_cameraAimLine);
	}
}
