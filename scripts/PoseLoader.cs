using System.IO;
using Godot;

// The engine-side half of loading: reads res:// files and hands plain
// readers to PoseData, which knows nothing about Godot's filesystem.
public static class PoseLoader
{
	public static PoseData LoadFromResources()
	{
		string posesText = ReadText("res://data/poses.txt");
		string namesText = ReadText("res://data/joint-names.txt");
		string focalText = ReadText("res://data/focal.txt");

		StringReader poses = new StringReader(posesText);
		StringReader names = new StringReader(namesText);
		PoseData data = PoseData.Parse(poses, names, focalText);
		poses.Dispose();
		names.Dispose();
		return data;
	}

	private static string ReadText(string path)
	{
		Godot.FileAccess file = Godot.FileAccess.Open(path, Godot.FileAccess.ModeFlags.Read);
		if (file == null)
		{
			GD.PushError("could not open " + path);
			return "";
		}
		string text = file.GetAsText();
		file.Dispose();
		return text;
	}
}
