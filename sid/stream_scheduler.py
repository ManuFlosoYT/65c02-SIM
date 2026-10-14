"""
stream_scheduler.py - Compile-time SD read scheduler for microDOS play.app.
Simulates decompressor consumption and plans exact SD read events during pauses.
"""

SD_DEFAULT_COST = 49731  # cycles per 512B read (NTSC 1MHz, recommended by sdbench)
DEFAULT_TARGET_BUFS = 16 # Buffers in play.c ring buffer (8KB footprint)
CHUNK_SIZE = 512


def schedule_stream_reads(compressed_stream_len, pause_events, target_bufs=DEFAULT_TARGET_BUFS, sd_cost=SD_DEFAULT_COST):
    """
    pause_events: list of tuples: (pause_index, cycles, bytes_consumed_before_this_pause)
    Returns:
        sched_table: bytearray of scheduled read entries
        stats: dict of simulation statistics (total_reads, forced_reads, max_delay_ms, min_slack)
    """
    total_blocks = (compressed_stream_len + CHUNK_SIZE - 1) // CHUNK_SIZE
    if total_blocks == 0:
        return bytearray(), {'total_reads': 0, 'forced_reads': 0, 'max_delay_ms': 0, 'min_bufs': 1}

    # Pre-roll: fills min(total_blocks, target_bufs)
    pre_roll_blocks = min(total_blocks, target_bufs)
    buffered_blocks = pre_roll_blocks
    consumed_blocks = 0

    planned_reads = []  # list of (pause_index, blocks_to_read)
    min_slack = target_bufs
    forced_reads = 0
    max_overdue_cycles = 0

    last_scheduled_pause = -1

    for p_idx, cycles, bytes_consumed in pause_events:
        consumed_blocks = bytes_consumed // CHUNK_SIZE
        slack = buffered_blocks - consumed_blocks
        if slack < min_slack:
            min_slack = slack

        if buffered_blocks >= total_blocks:
            continue

        free_bufs = target_bufs - (buffered_blocks - consumed_blocks)
        if free_bufs <= 0:
            continue

        # How many blocks can fit in this pause?
        can_read = int(cycles // sd_cost)
        to_read = min(free_bufs, can_read, 4, total_blocks - buffered_blocks)

        if to_read > 0:
            planned_reads.append((p_idx, to_read))
            buffered_blocks += to_read
            last_scheduled_pause = p_idx
        elif slack <= 2 and (total_blocks - buffered_blocks) > 0:
            # Emergency read if buffer is running low
            to_read = min(free_bufs, 1, total_blocks - buffered_blocks)
            planned_reads.append((p_idx, to_read))
            buffered_blocks += to_read
            forced_reads += 1
            overdue = sd_cost - cycles if sd_cost > cycles else 0
            if overdue > max_overdue_cycles:
                max_overdue_cycles = overdue

    # Encode planned reads into sched_table byte stream
    sched_table = bytearray()
    cur_p = 0

    for p_idx, blocks in planned_reads:
        diff = p_idx - cur_p
        while diff >= 63:
            # Escape entry: skip 63 pauses, read 0 blocks (encoded as blocks-1 = 0? No, escape: blocks=1, but bit pattern)
            # Bits 7-6: blocks - 1. Bits 5-0: skip.
            # If skip == 63, it adds 63 to skip counter without triggering a read!
            # We define: skip == 63 with blocks_flag == 0 means EXTEND skip by 63, NO READ.
            sched_table.append(0x3F)  # blocks=0 (flag 00), skip=63 (111111)
            diff -= 63

        # Read entry: (blocks - 1) << 6 | (diff & 0x3F)
        val = ((blocks - 1) & 0x03) << 6 | (diff & 0x3F)
        sched_table.append(val)
        cur_p = p_idx + 1

    min_bufs_required = max(4, target_bufs - min_slack + 2)
    min_bufs_required = min(min_bufs_required, target_bufs)

    stats = {
        'total_reads': sum(b for _, b in planned_reads) + pre_roll_blocks,
        'planned_read_events': len(planned_reads),
        'pre_roll_blocks': pre_roll_blocks,
        'forced_reads': forced_reads,
        'max_delay_ms': (max_overdue_cycles / 1000.0),
        'min_bufs_required': min_bufs_required
    }

    return sched_table, stats

