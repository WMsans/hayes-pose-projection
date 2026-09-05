using System.IO;
using System.Threading.Tasks;
using Godot;

// Produces the challenge deliverables. Recorded coordinates are always true
// image pixels; only the white-background images are rescaled to be legible.
public class Exporter
{
	private Main _main;

	public Exporter(Main main)
	{
		_main = main;
	}

	public static string OutputDirectory()
	{
		string dir = ProjectSettings.GlobalizePath("res://out");
		Directory.CreateDirectory(dir);
		return dir;
	}

	public void WriteTables()
	{
		int count = _main.Data.Frames.Length;
		ProjectedFrame[] manual = new ProjectedFrame[count];
		ProjectedFrame[] godot = new ProjectedFrame[count];
		ProjectedFrame[] active = new ProjectedFrame[count];

		int restore = _main.FrameIndex;
		for (int f = 0; f < count; f++)
		{
			_main.ShowFrame(f);

			manual[f] = new ProjectedFrame();
			manual[f].Frame = f;
			manual[f].Points = _main.ProjectCurrent(ProjectionMethod.ManualPinhole);

			godot[f] = new ProjectedFrame();
			godot[f].Frame = f;
			godot[f].Points = _main.ProjectCurrent(ProjectionMethod.GodotUnproject);

			active[f] = new ProjectedFrame();
			active[f].Frame = f;
			active[f].Points = _main.ProjectCurrent(_main.Method);
		}
		_main.ShowFrame(restore);

		string dir = OutputDirectory();
		StreamWriter wide = new StreamWriter(Path.Combine(dir, "coords_wide.csv"));
		CoordinateTable.WriteWide(wide, active);
		wide.Dispose();

		StreamWriter longTable = new StreamWriter(Path.Combine(dir, "coords_long.csv"));
		CoordinateTable.WriteLong(longTable, manual, godot, _main.Data.JointNames);
		longTable.Dispose();

		StreamWriter agreement = new StreamWriter(Path.Combine(dir, "method_agreement.csv"));
		CoordinateTable.WriteAgreement(agreement, manual, godot);
		agreement.Dispose();
	}

	public async Task WriteImages()
	{
		string dir = OutputDirectory();

		bool wasTwoD = _main.TwoDimensionalView;
		ViewFraming wasMode = _main.Overlay.Mode;
		bool wasPhoto = _main.Overlay.ShowPhotograph;
		bool wasPanelVisible = _main.Panel.Visible;
		int restore = _main.FrameIndex;

		try
		{
			// The panel would otherwise be baked into every exported image.
			_main.Panel.Visible = false;
			_main.SetTwoDimensionalView(true);

			await Capture(dir, "proj_", ViewFraming.FitToPose, false);
			await Capture(dir, "overlay_", ViewFraming.ImagePixels, true);
		}
		finally
		{
			_main.Overlay.Mode = wasMode;
			_main.Overlay.ShowPhotograph = wasPhoto;
			_main.ShowFrame(restore);
			_main.SetTwoDimensionalView(wasTwoD);
			_main.Panel.Visible = wasPanelVisible;
		}
	}

	private async Task Capture(string dir, string prefix, ViewFraming mode, bool photograph)
	{
		_main.Overlay.Mode = mode;
		_main.Overlay.ShowPhotograph = photograph;

		for (int f = 0; f < _main.Data.Frames.Length; f++)
		{
			_main.ShowFrame(f);
			_main.Overlay.QueueRedraw();

			// Two frames, not one: the first lets the redraw be submitted,
			// the second guarantees it has been presented before the read.
			await _main.ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);
			await _main.ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);

			Image image = _main.GetViewport().GetTexture().GetImage();
			image.SavePng(Path.Combine(dir, prefix + f.ToString("00") + ".png"));
		}
	}
}
