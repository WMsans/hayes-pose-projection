using Godot;

// Every operation other than moving the camera lives here. No shortcut
// keys: the keyboard belongs to the fly camera.
public partial class ControlPanel : PanelContainer
{
	private Main _main;
	private Label _frameLabel;
	private HSlider _frameSlider;
	private OptionButton _viewOption;
	private OptionButton _methodOption;
	private OptionButton _framingOption;
	private OptionButton _backgroundOption;
	private Label _agreementLabel;
	private Label _statusLabel;

	public void Build(Main main)
	{
		_main = main;

		SetAnchorsPreset(Control.LayoutPreset.TopLeft);
		Position = new Vector2(12.0f, 12.0f);
		CustomMinimumSize = new Vector2(280.0f, 0.0f);

		VBoxContainer box = new VBoxContainer();
		AddChild(box);

		_frameLabel = new Label();
		box.AddChild(_frameLabel);

		HBoxContainer stepper = new HBoxContainer();
		box.AddChild(stepper);

		Button previous = new Button();
		previous.Text = "<";
		previous.Pressed += OnPrevious;
		stepper.AddChild(previous);

		_frameSlider = new HSlider();
		_frameSlider.MinValue = 0;
		_frameSlider.MaxValue = _main.Data.Frames.Length - 1;
		_frameSlider.Step = 1;
		_frameSlider.CustomMinimumSize = new Vector2(180.0f, 0.0f);
		_frameSlider.ValueChanged += OnSliderChanged;
		stepper.AddChild(_frameSlider);

		Button next = new Button();
		next.Text = ">";
		next.Pressed += OnNext;
		stepper.AddChild(next);

		_viewOption = AddOption(box, "View", new string[] { "3D scene", "2D projection" });
		_viewOption.ItemSelected += OnViewSelected;

		_methodOption = AddOption(box, "Projection method", new string[] { "Manual pinhole", "Godot unproject" });
		_methodOption.Selected = (int)_main.Method;
		_methodOption.ItemSelected += OnMethodSelected;

		_framingOption = AddOption(box, "Framing", new string[] { "Fit to pose", "True image pixels" });
		_framingOption.ItemSelected += OnFramingSelected;

		_backgroundOption = AddOption(box, "Background", new string[] { "White", "Photograph" });
		_backgroundOption.ItemSelected += OnBackgroundSelected;

		Button snap = new Button();
		snap.Text = "Snap to challenge camera";
		snap.Pressed += OnSnap;
		box.AddChild(snap);

		Button export = new Button();
		export.Text = "Export tables and images";
		export.Pressed += OnExport;
		box.AddChild(export);

		_agreementLabel = new Label();
		box.AddChild(_agreementLabel);

		_statusLabel = new Label();
		_statusLabel.AutowrapMode = TextServer.AutowrapMode.WordSmart;
		_statusLabel.CustomMinimumSize = new Vector2(260.0f, 0.0f);
		box.AddChild(_statusLabel);

		Label help = new Label();
		help.Text = "WASD move, Q/E down/up, Shift faster,\nclick to look, Esc frees the cursor";
		box.AddChild(help);

		UpdateReadouts();
	}

	public void SetStatus(string text)
	{
		_statusLabel.Text = text;
	}

	public void UpdateReadouts()
	{
		_frameLabel.Text = "Frame " + (_main.FrameIndex + 1).ToString("00") + " / " + _main.Data.Frames.Length;
		_frameSlider.SetValueNoSignal(_main.FrameIndex);

		ProjectedFrame manual = new ProjectedFrame();
		manual.Frame = _main.FrameIndex;
		manual.Points = _main.ProjectCurrent(ProjectionMethod.ManualPinhole);

		ProjectedFrame godot = new ProjectedFrame();
		godot.Frame = _main.FrameIndex;
		godot.Points = _main.ProjectCurrent(ProjectionMethod.GodotUnproject);

		float worst = CoordinateTable.MaxAbsDiff(manual, godot);
		_agreementLabel.Text = "Methods agree to " + worst.ToString("0.0000") + " px";

		_framingOption.Disabled = _main.Overlay.ShowPhotograph;
	}

	private static OptionButton AddOption(VBoxContainer box, string caption, string[] items)
	{
		Label label = new Label();
		label.Text = caption;
		box.AddChild(label);

		OptionButton option = new OptionButton();
		for (int i = 0; i < items.Length; i++)
		{
			option.AddItem(items[i], i);
		}
		option.Selected = 0;
		box.AddChild(option);
		return option;
	}

	private void OnPrevious()
	{
		_main.ShowFrame(_main.FrameIndex - 1);
		UpdateReadouts();
	}

	private void OnNext()
	{
		_main.ShowFrame(_main.FrameIndex + 1);
		UpdateReadouts();
	}

	private void OnSliderChanged(double value)
	{
		_main.ShowFrame((int)value);
		UpdateReadouts();
	}

	private void OnViewSelected(long index)
	{
		_main.SetTwoDimensionalView(index == 1);
		UpdateReadouts();
	}

	private void OnMethodSelected(long index)
	{
		_main.SetProjectionMethod((ProjectionMethod)index);
		UpdateReadouts();
	}

	private void OnFramingSelected(long index)
	{
		_main.Overlay.Mode = (ViewFraming)index;
		_main.Overlay.QueueRedraw();
	}

	private void OnBackgroundSelected(long index)
	{
		_main.Overlay.ShowPhotograph = index == 1;
		_main.Overlay.QueueRedraw();
		UpdateReadouts();
	}

	private void OnSnap()
	{
		_main.Fly.SnapTo(_main.ChallengeCam);
	}

	private async void OnExport()
	{
		SetStatus("exporting...");
		string message = await _main.RunExport();
		SetStatus(message);
	}
}
