Get-Process | Where-Object { $_.Name -match 'Server|msbuild|cl$|link|devenv|VBCSCompiler' } | Select-Object Id,Name | Format-Table -AutoSize
