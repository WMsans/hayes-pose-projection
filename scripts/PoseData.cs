using System.Collections.Generic;
using System.Globalization;
using System.IO;
using CsvHelper;
using CsvHelper.Configuration;
using Godot;

// Parses the three text files from Pose.zip. Takes readers rather than
// paths so it can be tested without the engine's res:// filesystem.
public class PoseData
{
	public const int JointCount = 14;

	public float Focal;
	public string[] JointNames;
	public PoseFrame[] Frames;

	public static PoseData Parse(TextReader posesReader, TextReader jointNamesReader, string focalText)
	{
		PoseData data = new PoseData();
		data.Focal = float.Parse(focalText.Trim(), CultureInfo.InvariantCulture);
		data.JointNames = ParseJointNames(jointNamesReader);
		data.Frames = ParsePoses(posesReader);
		return data;
	}

	private static string[] ParseJointNames(TextReader reader)
	{
		List<string> names = new List<string>();
		string line = reader.ReadLine();
		while (line != null)
		{
			string trimmed = line.Trim();
			if (trimmed.Length > 0)
			{
				// each line is "0   'Hip'"
				int quote = trimmed.IndexOf('\'');
				string name = trimmed.Substring(quote + 1).TrimEnd('\'');
				names.Add(name);
			}
			line = reader.ReadLine();
		}
		return names.ToArray();
	}

	private static PoseFrame[] ParsePoses(TextReader reader)
	{
		CsvConfiguration config = new CsvConfiguration(CultureInfo.InvariantCulture);
		config.Delimiter = "\t";
		config.HasHeaderRecord = false;

		List<PoseFrame> frames = new List<PoseFrame>();
		CsvReader csv = new CsvReader(reader, config);
		while (csv.Read())
		{
			float[] values = new float[3 + JointCount * 3];
			for (int i = 0; i < values.Length; i++)
			{
				values[i] = csv.GetField<float>(i);
			}

			PoseFrame frame = new PoseFrame();
			frame.CameraWorld = new Vector3(values[0], values[1], values[2]);
			frame.JointsWorld = new Vector3[JointCount];
			for (int j = 0; j < JointCount; j++)
			{
				int at = 3 + j * 3;
				frame.JointsWorld[j] = new Vector3(values[at], values[at + 1], values[at + 2]);
			}
			frame.ComputeDerived();
			frames.Add(frame);
		}
		return frames.ToArray();
	}
}
