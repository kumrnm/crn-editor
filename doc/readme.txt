【CRNエディタ 1.0】

CRN静画規格 1.0に準拠した静画素材の正規化を行うソフトウェアです。
crn-editor.exeとcrn-normalizer.exeは原則として同じフォルダに配置してください。


■ crn-editor.exe
pngファイルの正規化用補助データであるcrnファイルを編集するGUIツールです。
pngファイルをcrn-editor.exeにドラッグ&ドロップして編集を開始します。
キャラクターの頭部を表す円と中心線を表す直線（いずれも青色）をドラッグ&ドロップで移動するほか、以下の操作がサポートされます。
- 青い円をShift+ドラッグ: 円の大きさを変更
- 「2」「3」キー: 中心線のタイプを変更（ショートカット）
- Shift+矢印キー: キャラクターの向きの指定を変更（ショートカット）
- 左右キー: 同一フォルダの他のpng画像を開く
XXX.pngの補助データはXXX.png.crnに自動的に保存されます。


■ crn-normalizer.exe
crnファイルの内容に基づいてpng画像を正規化するCLIツールです。
crn-editor.exeから内部的に呼び出されるので、基本的にこちらを直接使用する必要はありません。
使用法は crn-normalizer.exe --help で閲覧できます。



当ソフトウェアのソースコードは以下のページで入手できます。
https://github.com/kumrnm/crn-editor
