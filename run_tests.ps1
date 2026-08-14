$tests = Get-ChildItem -Path build\test_*.exe -Exclude "test_bonus_features.exe"
$failed = 0

Write-Host "=== AI Agent Framework: Running All Unit Tests ===" -ForegroundColor Cyan

foreach ($test in $tests) {
    Write-Host "`n>>> Running $($test.Name)..." -ForegroundColor Yellow
    & $test.FullName
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[FAIL] FAILED: $($test.Name) exited with code $LASTEXITCODE" -ForegroundColor Red
        $failed++
    } else {
        Write-Host "[PASS] PASSED: $($test.Name)" -ForegroundColor Green
    }
}

Write-Host "`n==================================================" -ForegroundColor Cyan
if ($failed -eq 0) {
    Write-Host "SUCCESS: All tests passed flawlessly! (100% Pass Rate)" -ForegroundColor Green
    exit 0
} else {
    Write-Host "ERROR: $failed test executable(s) failed." -ForegroundColor Red
    exit 1
}
