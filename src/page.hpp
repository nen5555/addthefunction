#pragma once

// 网页前端：一个自包含的 HTML 页面，数据从 /api/status 获取
// R"HTML( ... )HTML" 是 C++ 原始字符串字面量（raw string literal），里面的引号、换行都不用转义
inline const char* kIndexHtml = R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-title" content="阅读目标">
<title>阅读目标</title>
<style>
  :root {
    --bg: #f4f5f2; --card: #ffffff; --text: #1d2420; --muted: #6b746e;
    --accent: #2f9e6e; --track: #e3e7e4; --cell0: #e6eae7;
    --cell1: #b7e2cb; --cell2: #6fc79b; --cell3: #2f9e6e; --cell4: #1d6b49;
    --warn: #b4562c;
  }
  @media (prefers-color-scheme: dark) {
    :root {
      --bg: #121614; --card: #1b201d; --text: #e9eeea; --muted: #8d978f;
      --accent: #4ac694; --track: #2a312d; --cell0: #242a26;
      --cell1: #1f4a35; --cell2: #2a7350; --cell3: #3ba675; --cell4: #5fd9a2;
      --warn: #e08a5f;
    }
  }
  * { box-sizing: border-box; }
  body {
    margin: 0; background: var(--bg); color: var(--text);
    font: 15px/1.5 -apple-system, BlinkMacSystemFont, "PingFang SC", "Microsoft YaHei", sans-serif;
    padding: max(20px, env(safe-area-inset-top)) 16px 32px;
  }
  main { max-width: 520px; margin: 0 auto; display: grid; gap: 14px; }
  h1 { font-size: 20px; margin: 4px 0 2px; font-weight: 650; letter-spacing: .02em; }
  .sub { color: var(--muted); font-size: 13px; margin: 0; }
  .card { background: var(--card); border-radius: 18px; padding: 20px; }
  .ring-wrap { display: grid; place-items: center; position: relative; }
  .ring { width: 210px; height: 210px; transform: rotate(-90deg); }
  .ring circle { fill: none; stroke-width: 14; }
  .ring .track { stroke: var(--track); }
  .ring .bar { stroke: var(--accent); stroke-linecap: round; transition: stroke-dashoffset .8s ease; }
  .ring-center { position: absolute; text-align: center; }
  .ring-center .big { font-size: 46px; font-weight: 700; line-height: 1; font-variant-numeric: tabular-nums; }
  .ring-center .small { color: var(--muted); font-size: 13px; margin-top: 6px; }
  .status { text-align: center; margin-top: 10px; font-weight: 600; }
  .stats { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; }
  .stat { background: var(--card); border-radius: 14px; padding: 14px 10px; text-align: center; }
  .stat .v { font-size: 26px; font-weight: 700; font-variant-numeric: tabular-nums; }
  .stat .k { color: var(--muted); font-size: 12px; }
  .heat-title { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 12px; }
  .heat-title b { font-size: 14px; }
  .heat-title span { color: var(--muted); font-size: 12px; }
  .heat { display: grid; grid-auto-flow: column; grid-template-rows: repeat(7, 1fr); gap: 4px; }
  .cell { aspect-ratio: 1; border-radius: 4px; background: var(--cell0); }
  .cell.l1 { background: var(--cell1); } .cell.l2 { background: var(--cell2); }
  .cell.l3 { background: var(--cell3); } .cell.l4 { background: var(--cell4); }
  .cell.today { outline: 2px solid var(--text); outline-offset: 1px; }
  .legend { display: flex; gap: 4px; align-items: center; justify-content: flex-end; margin-top: 10px; color: var(--muted); font-size: 11px; }
  .legend .cell { width: 11px; }
  .foot { color: var(--muted); font-size: 12px; text-align: center; }
  .error { background: var(--card); border-left: 4px solid var(--warn); border-radius: 12px; padding: 12px 14px; display: none; }
</style>
</head>
<body>
<main>
  <header>
    <h1>阅读目标</h1>
    <p class="sub" id="date">—</p>
  </header>

  <div class="error" id="error"></div>

  <section class="card">
    <div class="ring-wrap">
      <svg class="ring" viewBox="0 0 200 200" aria-hidden="true">
        <circle class="track" cx="100" cy="100" r="86"></circle>
        <circle class="bar" id="bar" cx="100" cy="100" r="86"></circle>
      </svg>
      <div class="ring-center">
        <div class="big" id="minutes">0</div>
        <div class="small" id="goal">/ 10 分钟</div>
      </div>
    </div>
    <div class="status" id="status">—</div>
  </section>

  <section class="stats">
    <div class="stat"><div class="v" id="streak">0</div><div class="k">连续天数</div></div>
    <div class="stat"><div class="v" id="best">0</div><div class="k">最长连续</div></div>
    <div class="stat"><div class="v" id="week">0</div><div class="k">近 7 天分钟</div></div>
  </section>

  <section class="card">
    <div class="heat-title"><b>最近 16 周</b><span id="heatnote"></span></div>
    <div class="heat" id="heat"></div>
    <div class="legend">少
      <div class="cell"></div><div class="cell l1"></div><div class="cell l2"></div>
      <div class="cell l3"></div><div class="cell l4"></div>多</div>
  </section>

  <p class="foot" id="foot">数据来自 Kavita，读完后约 10–15 分钟才会计入</p>
</main>

<script>
const R = 86, C = 2 * Math.PI * R;
const bar = document.getElementById('bar');
bar.style.strokeDasharray = C;
bar.style.strokeDashoffset = C;

function level(sec, goalSec) {
  if (sec <= 0) return 0;
  if (sec < goalSec * 0.5) return 1;
  if (sec < goalSec) return 2;
  if (sec < goalSec * 2) return 3;
  return 4;
}

function render(d) {
  const goalSec = d.goalMinutes * 60;
  const mins = Math.floor(d.todaySeconds / 60);
  const ratio = Math.min(d.todaySeconds / goalSec, 1);
  bar.style.strokeDashoffset = C * (1 - ratio);

  const wk = ['日', '一', '二', '三', '四', '五', '六'];
  const t = new Date(d.today + 'T12:00:00');
  document.getElementById('date').textContent = d.today + ' 星期' + wk[t.getDay()];
  document.getElementById('minutes').textContent = mins;
  document.getElementById('goal').textContent = '/ ' + d.goalMinutes + ' 分钟';
  document.getElementById('status').textContent = d.todaySeconds >= goalSec
    ? '今日目标已完成 ✅'
    : '还差 ' + Math.ceil((goalSec - d.todaySeconds) / 60) + ' 分钟';
  document.getElementById('streak').textContent = d.streak;
  document.getElementById('best').textContent = d.best;
  document.getElementById('week').textContent = Math.floor(d.weekSeconds / 60);

  // 热力图：每列一周（周一到周日），最后一列包含今天
  const heat = document.getElementById('heat');
  heat.innerHTML = '';
  // 去掉开头不足一周的几天，让第一列从周一开始
  const dow = s => (new Date(s + 'T12:00:00').getDay() + 6) % 7;  // 周一 = 0
  let days = d.days;
  while (days.length && dow(days[0].date) !== 0) days = days.slice(1);
  let readDays = 0;
  for (const day of days) {
    const c = document.createElement('div');
    const lv = level(day.seconds, goalSec);
    c.className = 'cell' + (lv ? ' l' + lv : '') + (day.date === d.today ? ' today' : '');
    c.title = day.date + '：' + Math.floor(day.seconds / 60) + ' 分钟';
    heat.appendChild(c);
    if (day.seconds > 0) readDays++;
  }
  document.getElementById('heatnote').textContent = '读书 ' + readDays + ' 天';
  document.getElementById('foot').textContent =
    '更新于 ' + d.updatedAt + ' · 读完后约 10–15 分钟才会计入';
}

async function load() {
  const err = document.getElementById('error');
  try {
    const r = await fetch('api/status', { cache: 'no-store' });
    const d = await r.json();
    if (!r.ok) throw new Error(d.error || ('HTTP ' + r.status));
    err.style.display = 'none';
    render(d);
  } catch (e) {
    err.textContent = '获取数据失败：' + e.message;
    err.style.display = 'block';
  }
}

load();
setInterval(load, 60 * 1000);
document.addEventListener('visibilitychange', () => { if (!document.hidden) load(); });
</script>
</body>
</html>
)HTML";
