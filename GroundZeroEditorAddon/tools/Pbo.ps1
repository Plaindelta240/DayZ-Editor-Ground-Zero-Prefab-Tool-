if (-not ('PboTool' -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Security.Cryptography;

public static class PboTool
{
    static string ReadZ(BinaryReader r)
    {
        var sb = new List<byte>();
        byte b;
        while ((b = r.ReadByte()) != 0) sb.Add(b);
        return Encoding.UTF8.GetString(sb.ToArray());
    }

    public static string Extract(string pbo, string outDir)
    {
        var log = new StringBuilder();
        using (var fs = File.OpenRead(pbo))
        using (var r = new BinaryReader(fs))
        {
            var entries = new List<object[]>();
            while (true)
            {
                string name = ReadZ(r);
                uint method = r.ReadUInt32();
                uint origSize = r.ReadUInt32();
                uint reserved = r.ReadUInt32();
                uint ts = r.ReadUInt32();
                uint dataSize = r.ReadUInt32();
                if (name == "" && method == 0x56657273) // 'sreV' product entry
                {
                    // header extension: key/value strings until empty key
                    while (true)
                    {
                        string k = ReadZ(r);
                        if (k == "") break;
                        string v = ReadZ(r);
                        log.AppendLine("  prop: " + k + " = " + v);
                    }
                    continue;
                }
                if (name == "" ) break;
                entries.Add(new object[] { name, dataSize });
            }
            foreach (var e in entries)
            {
                string name = (string)e[0];
                uint size = (uint)e[1];
                byte[] data = r.ReadBytes((int)size);
                string dest = Path.Combine(outDir, name.Replace('\\', Path.DirectorySeparatorChar));
                Directory.CreateDirectory(Path.GetDirectoryName(dest));
                File.WriteAllBytes(dest, data);
            }
            log.AppendLine("extracted " + entries.Count + " files");
        }
        return log.ToString();
    }

    // files: relative path (using \) -> absolute source path
    public static void Pack(string outPbo, string prefix, SortedDictionary<string, string> files)
    {
        using (var ms = new MemoryStream())
        using (var w = new BinaryWriter(ms))
        {
            // product entry
            w.Write((byte)0);
            w.Write((uint)0x56657273);
            w.Write((uint)0); w.Write((uint)0); w.Write((uint)0); w.Write((uint)0);
            if (!string.IsNullOrEmpty(prefix))
            {
                w.Write(Encoding.UTF8.GetBytes("prefix")); w.Write((byte)0);
                w.Write(Encoding.UTF8.GetBytes(prefix)); w.Write((byte)0);
            }
            w.Write((byte)0);

            var datas = new List<byte[]>();
            foreach (var kv in files)
            {
                byte[] d = File.ReadAllBytes(kv.Value);
                datas.Add(d);
                w.Write(Encoding.UTF8.GetBytes(kv.Key)); w.Write((byte)0);
                w.Write((uint)0);             // uncompressed
                w.Write((uint)d.Length);      // original size
                w.Write((uint)0);
                w.Write((uint)0);             // timestamp
                w.Write((uint)d.Length);      // data size
            }
            // terminator entry
            w.Write((byte)0);
            w.Write((uint)0); w.Write((uint)0); w.Write((uint)0); w.Write((uint)0); w.Write((uint)0);

            foreach (var d in datas) w.Write(d);

            w.Flush();
            byte[] body = ms.ToArray();
            using (var sha = SHA1.Create())
            {
                byte[] hash = sha.ComputeHash(body);
                using (var o = File.Create(outPbo))
                {
                    o.Write(body, 0, body.Length);
                    o.WriteByte(0);
                    o.Write(hash, 0, hash.Length);
                }
            }
        }
    }
}
'@
}
