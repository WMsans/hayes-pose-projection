using System.Globalization;
using System.IO;
using CsvHelper;
using Godot;

public class ProjectedFrame
{
	public int Frame;
	public Vector2[] Points;
}

// The challenge asks for every 2D coordinate in one table. These are always
// true image pixels, never the fit-to-pose display coordinates.
public static class CoordinateTable
{
	private const string Format = "0.000";

	public static void WriteWide(TextWriter writer, ProjectedFrame[] frames)
	{
		CsvWriter csv = new CsvWriter(writer, CultureInfo.InvariantCulture, true);
		csv.WriteField("frame");
		for (int j = 0; j < PoseData.JointCount; j++)
		{
			csv.WriteField("u" + j);
			csv.WriteField("v" + j);
		}
		csv.NextRecord();

		for (int f = 0; f < frames.Length; f++)
		{
			csv.WriteField(frames[f].Frame);
			for (int j = 0; j < PoseData.JointCount; j++)
			{
				csv.WriteField(Text(frames[f].Points[j].X));
				csv.WriteField(Text(frames[f].Points[j].Y));
			}
			csv.NextRecord();
		}
		csv.Flush();
	}

	public static void WriteLong(TextWriter writer, ProjectedFrame[] manual, ProjectedFrame[] godot, string[] jointNames)
	{
		CsvWriter csv = new CsvWriter(writer, CultureInfo.InvariantCulture, true);
		csv.WriteField("frame");
		csv.WriteField("joint_id");
		csv.WriteField("joint_name");
		csv.WriteField("method");
		csv.WriteField("u");
		csv.WriteField("v");
		csv.NextRecord();

		WriteLongRows(csv, manual, jointNames, "manual_pinhole");
		WriteLongRows(csv, godot, jointNames, "godot_unproject");
		csv.Flush();
	}

	private static void WriteLongRows(CsvWriter csv, ProjectedFrame[] frames, string[] jointNames, string method)
	{
		for (int f = 0; f < frames.Length; f++)
		{
			for (int j = 0; j < PoseData.JointCount; j++)
			{
				csv.WriteField(frames[f].Frame);
				csv.WriteField(j);
				csv.WriteField(jointNames[j]);
				csv.WriteField(method);
				csv.WriteField(Text(frames[f].Points[j].X));
				csv.WriteField(Text(frames[f].Points[j].Y));
				csv.NextRecord();
			}
		}
	}

	public static void WriteAgreement(TextWriter writer, ProjectedFrame[] manual, ProjectedFrame[] godot)
	{
		CsvWriter csv = new CsvWriter(writer, CultureInfo.InvariantCulture, true);
		csv.WriteField("frame");
		csv.WriteField("max_abs_diff_px");
		csv.WriteField("mean_abs_diff_px");
		csv.NextRecord();

		for (int f = 0; f < manual.Length; f++)
		{
			csv.WriteField(manual[f].Frame);
			csv.WriteField(MaxAbsDiff(manual[f], godot[f]).ToString("0.000000", CultureInfo.InvariantCulture));
			csv.WriteField(MeanAbsDiff(manual[f], godot[f]).ToString("0.000000", CultureInfo.InvariantCulture));
			csv.NextRecord();
		}
		csv.Flush();
	}

	public static float MaxAbsDiff(ProjectedFrame a, ProjectedFrame b)
	{
		float worst = 0.0f;
		for (int j = 0; j < a.Points.Length; j++)
		{
			worst = Mathf.Max(worst, Mathf.Abs(a.Points[j].X - b.Points[j].X));
			worst = Mathf.Max(worst, Mathf.Abs(a.Points[j].Y - b.Points[j].Y));
		}
		return worst;
	}

	public static float MeanAbsDiff(ProjectedFrame a, ProjectedFrame b)
	{
		float total = 0.0f;
		for (int j = 0; j < a.Points.Length; j++)
		{
			total += Mathf.Abs(a.Points[j].X - b.Points[j].X);
			total += Mathf.Abs(a.Points[j].Y - b.Points[j].Y);
		}
		return total / (a.Points.Length * 2);
	}

	private static string Text(float value)
	{
		return value.ToString(Format, CultureInfo.InvariantCulture);
	}
}
