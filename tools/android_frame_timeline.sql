-- Run against the local API35 trace using Perfetto Trace Processor.
-- Negative durations are incomplete slices, not zero-duration good frames.
SELECT process.name, jank_type, present_type, on_time_finish,
       COUNT(*) AS frames, SUM(dur < 0) AS incomplete,
       ROUND(AVG(CASE WHEN dur >= 0 THEN dur END)/1e6,3) AS completed_mean_ms
FROM actual_frame_timeline_slice LEFT JOIN process USING(upid)
WHERE process.name LIKE '%pocketengineer%' OR process.name='/system/bin/surfaceflinger'
GROUP BY 1,2,3,4;

-- Frame completion latency is not the interval between displayed frames.
-- Retain long journey gaps, including gaps between injected gestures.
WITH displays AS (
 SELECT DISTINCT sf.display_frame_token, sf.ts+sf.dur AS presented
 FROM actual_frame_timeline_slice a
 JOIN process p ON a.upid=p.upid
 JOIN actual_frame_timeline_slice sf ON a.display_frame_token=sf.display_frame_token
 JOIN process sp ON sf.upid=sp.upid
 WHERE p.name='com.pocketengineer.app' AND sp.name='/system/bin/surfaceflinger'
       AND sf.dur>0 AND a.dur>0
), gaps AS (
 SELECT (presented-LAG(presented) OVER(ORDER BY presented))/1e6 AS ms FROM displays
)
SELECT COUNT(ms) AS displayed_intervals, MIN(ms) AS min_ms,
       PERCENTILE(ms,50) AS median_ms, PERCENTILE(ms,95) AS p95_ms, MAX(ms) AS max_ms
FROM gaps;
