inter_frame_mode_info( ) {
     use_intrabc = 0
     LeftRefFrame[ 0 ] = AvailL ? RefFrames[ MiRow ][ MiCol-1 ][ 0 ] : INTRA_FRAME
     AboveRefFrame[ 0 ] = AvailU ? RefFrames[ MiRow-1 ][ MiCol ][ 0 ] : INTRA_FRAME
     LeftRefFrame[ 1 ] = AvailL ? RefFrames[ MiRow ][ MiCol-1 ][ 1 ] : NONE
     AboveRefFrame[ 1 ] = AvailU ? RefFrames[ MiRow-1 ][ MiCol ][ 1 ] : NONE
     LeftIntra = LeftRefFrame[ 0 ] <= INTRA_FRAME
     AboveIntra = AboveRefFrame[ 0 ] <= INTRA_FRAME
     LeftSingle = LeftRefFrame[ 1 ] <= INTRA_FRAME
     AboveSingle = AboveRefFrame[ 1 ] <= INTRA_FRAME
     skip = 0
     inter_segment_id( 1 )
     read_skip_mode( )
     if ( skip_mode )
         skip = 1
     else
         read_skip( )
     if ( !SegIdPreSkip )
         inter_segment_id( 0 )
     Lossless = LosslessArray[ segment_id ]
     read_cdef( )
     read_delta_qindex( )
     read_delta_lf( )
     ReadDeltas = 0
     read_is_inter( )
     if ( is_inter )
         inter_block_mode_info( )
     else
         intra_block_mode_info( )
}



inter_segment_id( preSkip ) {
     if ( segmentation_enabled ) {
         predictedSegmentId = get_segment_id( )
         if ( segmentation_update_map ) {
             if ( preSkip && !SegIdPreSkip ) {
                 segment_id = 0
                 return
             }
             if ( !preSkip ) {
                 if ( skip ) {
                     seg_id_predicted = 0
                     for ( i = 0; i < Num_4x4_Blocks_Wide[ MiSize ]; i++ )
                         AboveSegPredContext[ MiCol + i ] = seg_id_predicted
                     for ( i = 0; i < Num_4x4_Blocks_High[ MiSize ]; i++ )
                         LeftSegPredContext[ MiRow + i ] = seg_id_predicted
                     read_segment_id( )
                     return
                 }
             }
             if ( segmentation_temporal_update == 1 ) {
                 S() seg_id_predicted;
                 if ( seg_id_predicted )
                     segment_id = predictedSegmentId
                 else
                     read_segment_id( )
                 for ( i = 0; i < Num_4x4_Blocks_Wide[ MiSize ]; i++ )
                     AboveSegPredContext[ MiCol + i ] = seg_id_predicted
                 for ( i = 0; i < Num_4x4_Blocks_High[ MiSize ]; i++ )
                     LeftSegPredContext[ MiRow + i ] = seg_id_predicted
             } else {
                 read_segment_id( )
             }
         } else {
             segment_id = predictedSegmentId
         }
     } else {
         segment_id = 0
     }
}



read_is_inter( ) {
     if ( skip_mode ) {
         is_inter = 1
     } else if ( seg_feature_active ( SEG_LVL_REF_FRAME ) ) {
         is_inter = FeatureData[ segment_id ][ SEG_LVL_REF_FRAME ] != INTRA_FRAME
     } else if ( seg_feature_active ( SEG_LVL_GLOBALMV ) ) {
         is_inter = 1
     } else {
         S() is_inter;
     }
}



get_segment_id( ) {
     bw4 = Num_4x4_Blocks_Wide[ MiSize ]
     bh4 = Num_4x4_Blocks_High[ MiSize ]
     xMis = Min( MiCols - MiCol, bw4 )
     yMis = Min( MiRows - MiRow, bh4 )
     seg = 7
     for ( y = 0; y < yMis; y++ )
         for ( x = 0; x < xMis; x++ )
             seg = Min( seg, PrevSegmentIds[ MiRow + y ][ MiCol + x ] )
     return seg
}



intra_block_mode_info( ) {
     RefFrame[ 0 ] = INTRA_FRAME
     RefFrame[ 1 ] = NONE
     S() y_mode;
     YMode = y_mode
     intra_angle_info_y( )
     if ( HasChroma ) {
         S() uv_mode;
         UVMode = uv_mode
         if ( UVMode == UV_CFL_PRED ) {
             read_cfl_alphas( )
         }
         intra_angle_info_uv( )
     }
     PaletteSizeY = 0
     PaletteSizeUV = 0
     if ( MiSize >= BLOCK_8X8 &&
          Block_Width[ MiSize ] <= 64  &&
          Block_Height[ MiSize ] <= 64 &&
          allow_screen_content_tools )
         palette_mode_info( )
     filter_intra_mode_info( )
}



inter_block_mode_info( ) {
     PaletteSizeY = 0
     PaletteSizeUV = 0
     read_ref_frames( )
     isCompound = RefFrame[ 1 ] > INTRA_FRAME
     find_mv_stack( isCompound )
     if ( skip_mode ) {
         YMode = NEAREST_NEARESTMV
     } else if ( seg_feature_active( SEG_LVL_SKIP ) ||
          seg_feature_active( SEG_LVL_GLOBALMV ) ) {
         YMode = GLOBALMV
     } else if ( isCompound ) {
         S() compound_mode;
         YMode = NEAREST_NEARESTMV + compound_mode
     } else {
         S() new_mv;
         if ( new_mv == 0 ) {
             YMode = NEWMV
         } else {
             S() zero_mv;
             if ( zero_mv == 0 ) {
                 YMode = GLOBALMV
             } else {
                 S() ref_mv;
                 YMode = (ref_mv == 0) ? NEARESTMV : NEARMV
             }
         }
     }
     RefMvIdx = 0
     if ( YMode == NEWMV || YMode == NEW_NEWMV ) {
         for ( idx = 0; idx < 2; idx++ ) {
             if ( NumMvFound > idx + 1 ) {
                 S() drl_mode;
                 if ( drl_mode == 0 ) {
                   RefMvIdx = idx
                   break
                 }
                 RefMvIdx = idx + 1
             }
         }
     } else if ( has_nearmv( ) ) {
         RefMvIdx = 1
         for ( idx = 1; idx < 3; idx++ ) {
             if ( NumMvFound > idx + 1 ) {
                 S() drl_mode;
                 if ( drl_mode == 0 ) {
                     RefMvIdx = idx
                     break
                 }
                 RefMvIdx = idx + 1
             }
         }
     }
     assign_mv( isCompound )
     read_interintra_mode( isCompound )
     read_motion_mode( isCompound )
     read_compound_type( isCompound )
     if ( interpolation_filter == SWITCHABLE ) {
         for ( dir = 0; dir < ( enable_dual_filter ? 2 : 1 ); dir++ ) {
             if ( needs_interp_filter( ) ) {
                 S() interp_filter[ dir ];
             } else {
                 interp_filter[ dir ] = EIGHTTAP
             }
         }
         if ( !enable_dual_filter )
             interp_filter[ 1 ] = interp_filter[ 0 ]
     } else {
         for ( dir = 0; dir < 2; dir++ )
             interp_filter[ dir ] = interpolation_filter
     }
}



filter_intra_mode_info( ) {
     use_filter_intra = 0
     if ( enable_filter_intra &&
          YMode == DC_PRED && PaletteSizeY == 0 &&
          Max( Block_Width[ MiSize ], Block_Height[ MiSize ] ) <= 32 ) {
         S() use_filter_intra;
         if ( use_filter_intra ) {
             S() filter_intra_mode;
         }
     }
}



read_ref_frames( ) {
     if ( skip_mode ) {
         RefFrame[ 0 ] = SkipModeFrame[ 0 ]
         RefFrame[ 1 ] = SkipModeFrame[ 1 ]
     } else if ( seg_feature_active( SEG_LVL_REF_FRAME ) ) {
         RefFrame[ 0 ] = FeatureData[ segment_id ][ SEG_LVL_REF_FRAME ]
         RefFrame[ 1 ] = NONE
     } else if ( seg_feature_active( SEG_LVL_SKIP ) ||
                 seg_feature_active( SEG_LVL_GLOBALMV ) ) {
         RefFrame[ 0 ] = LAST_FRAME
         RefFrame[ 1 ] = NONE
     } else {
         bw4 = Num_4x4_Blocks_Wide[ MiSize ]
         bh4 = Num_4x4_Blocks_High[ MiSize ]
         if ( reference_select && ( Min( bw4, bh4 ) >= 2 ) )
             S() comp_mode;
         else
             comp_mode = SINGLE_REFERENCE
         if ( comp_mode == COMPOUND_REFERENCE ) {
             S() comp_ref_type;
             if ( comp_ref_type == UNIDIR_COMP_REFERENCE ) {
                 S() uni_comp_ref;
                 if ( uni_comp_ref ) {
                     RefFrame[0] = BWDREF_FRAME
                     RefFrame[1] = ALTREF_FRAME
                 } else {
                     S() uni_comp_ref_p1;
                     if ( uni_comp_ref_p1 ) {
                         S() uni_comp_ref_p2;
                         if ( uni_comp_ref_p2 ) {
                           RefFrame[0] = LAST_FRAME
                           RefFrame[1] = GOLDEN_FRAME
                         } else {
                           RefFrame[0] = LAST_FRAME
                           RefFrame[1] = LAST3_FRAME
                         }
                     } else {
                         RefFrame[0] = LAST_FRAME
                         RefFrame[1] = LAST2_FRAME
                     }
                 }
             } else {
                 S() comp_ref;
                 if ( comp_ref == 0 ) {
                     S() comp_ref_p1;
                     RefFrame[ 0 ] = comp_ref_p1 ?
                                     LAST2_FRAME : LAST_FRAME
                 } else {
                     S() comp_ref_p2;
                     RefFrame[ 0 ] = comp_ref_p2 ?
                                     GOLDEN_FRAME : LAST3_FRAME
                 }
                 S() comp_bwdref;
                 if ( comp_bwdref == 0 ) {
                     S() comp_bwdref_p1;
                     RefFrame[ 1 ] = comp_bwdref_p1 ?
                                      ALTREF2_FRAME : BWDREF_FRAME
                 } else {
                     RefFrame[ 1 ] = ALTREF_FRAME
                 }
             }
         } else {
             S() single_ref_p1;
             if ( single_ref_p1 ) {
                 S() single_ref_p2;
                 if ( single_ref_p2 == 0 ) {
                     S() single_ref_p6;
                     RefFrame[ 0 ] = single_ref_p6 ?
                                      ALTREF2_FRAME : BWDREF_FRAME
                 } else {
                     RefFrame[ 0 ] = ALTREF_FRAME
                 }
             } else {
                 S() single_ref_p3;
                 if ( single_ref_p3 ) {
                     S() single_ref_p5;
                     RefFrame[ 0 ] = single_ref_p5 ?
                                      GOLDEN_FRAME : LAST3_FRAME
                 } else {
                     S() single_ref_p4;
                     RefFrame[ 0 ] = single_ref_p4 ?
                                      LAST2_FRAME : LAST_FRAME
                 }
             }
             RefFrame[ 1 ] = NONE
         }
     }
}



assign_mv( isCompound ) {
     for ( i = 0; i < 1 + isCompound; i++ ) {
         if ( use_intrabc ) {
             compMode = NEWMV
         } else {
             compMode = get_mode( i )
         }
         if ( use_intrabc ) {
             PredMv[ 0 ] = RefStackMv[ 0 ][ 0 ]
             if ( PredMv[ 0 ][ 0 ] == 0 && PredMv[ 0 ][ 1 ] == 0 ) {
                 PredMv[ 0 ] = RefStackMv[ 1 ][ 0 ]
             }
             if ( PredMv[ 0 ][ 0 ] == 0 && PredMv[ 0 ][ 1 ] == 0 ) {
                 sbSize = use_128x128_superblock ? BLOCK_128X128 : BLOCK_64X64
                 sbSize4 = Num_4x4_Blocks_High[ sbSize ]
                 if ( MiRow - sbSize4 < MiRowStart ) {
                     PredMv[ 0 ][ 0 ] = 0
                     PredMv[ 0 ][ 1 ] = -(sbSize4 * MI_SIZE + INTRABC_DELAY_PIXELS) * 8
                 } else {
                     PredMv[ 0 ][ 0 ] = -(sbSize4 * MI_SIZE * 8)
                     PredMv[ 0 ][ 1 ] = 0
                 }
             }
         } else if ( compMode == GLOBALMV ) {
             PredMv[ i ] = GlobalMvs[ i ]
         } else {
             pos = ( compMode == NEARESTMV ) ? 0 : RefMvIdx
             if ( compMode == NEWMV && NumMvFound <= 1 )
                 pos = 0
             PredMv[ i ] = RefStackMv[ pos ][ i ]
         }
         if ( compMode == NEWMV ) {
             read_mv( i )
         } else {
             Mv[ i ] = PredMv[ i ]
         }
     }
}



read_motion_mode( isCompound ) {
     if ( skip_mode ) {
         motion_mode = SIMPLE
         return
     }
     if ( !is_motion_mode_switchable ) {
         motion_mode = SIMPLE
         return
     }
     if ( Min( Block_Width[ MiSize ],
               Block_Height[ MiSize ] ) < 8 ) {
         motion_mode = SIMPLE
         return
     }
     if ( !force_integer_mv &&
          ( YMode == GLOBALMV || YMode == GLOBAL_GLOBALMV ) ) {
         if ( GmType[ RefFrame[ 0 ] ] > TRANSLATION ) {
             motion_mode = SIMPLE
             return
         }
     }
     if ( isCompound || RefFrame[ 1 ] == INTRA_FRAME || !has_overlappable_candidates( ) ) {
         motion_mode = SIMPLE
         return
     }
     find_warp_samples()
     if ( force_integer_mv || NumSamples == 0 ||
          !allow_warped_motion || is_scaled( RefFrame[0] ) ) {
         S() use_obmc;
         motion_mode = use_obmc ? OBMC : SIMPLE
     } else {
         S() motion_mode;
     }
}



read_interintra_mode( isCompound ) {
     if ( !skip_mode && enable_interintra_compound && !isCompound &&
          MiSize >= BLOCK_8X8 && MiSize <= BLOCK_32X32) {
         S() interintra;
         if ( interintra ) {
             S() interintra_mode;
             RefFrame[1] = INTRA_FRAME
             AngleDeltaY = 0
             AngleDeltaUV = 0
             use_filter_intra = 0
             S() wedge_interintra;
             if ( wedge_interintra ) {
                 S() wedge_index;
                 wedge_sign = 0
             }
         }
     } else {
         interintra = 0
     }
}



read_compound_type( isCompound ) {
     comp_group_idx = 0
     compound_idx = 1
     if ( skip_mode ) {
         compound_type = COMPOUND_AVERAGE
         return
     }
     if ( isCompound ) {
         n = Wedge_Bits[ MiSize ]
         if ( enable_masked_compound ) {
               S() comp_group_idx;
         }
         if ( comp_group_idx == 0 ) {
             if ( enable_jnt_comp ) {
                 S() compound_idx;
                 compound_type = compound_idx ? COMPOUND_AVERAGE :
                                                COMPOUND_DISTANCE
             } else {
                 compound_type = COMPOUND_AVERAGE
             }
         } else {
             if ( n == 0 ) {
                 compound_type = COMPOUND_DIFFWTD
             } else {
                 S() compound_type;
             }
         }
         if ( compound_type == COMPOUND_WEDGE ) {
             S() wedge_index;
             L(1) wedge_sign;
         } else if ( compound_type == COMPOUND_DIFFWTD ) {
             L(1) mask_type;
         }
     } else {
         if ( interintra ) {
             compound_type = wedge_interintra ? COMPOUND_WEDGE : COMPOUND_INTRA
         } else {
             compound_type = COMPOUND_AVERAGE
         }
     }
}



get_mode( refList ) {
     if ( refList == 0 ) {
         if ( YMode < NEAREST_NEARESTMV )
             compMode = YMode
         else if ( YMode == NEW_NEWMV || YMode == NEW_NEARESTMV || YMode == NEW_NEARMV )
             compMode = NEWMV
         else if ( YMode == NEAREST_NEARESTMV || YMode == NEAREST_NEWMV )
             compMode = NEARESTMV
         else if ( YMode == NEAR_NEARMV || YMode == NEAR_NEWMV )
             compMode = NEARMV
         else
             compMode = GLOBALMV
     } else {
         if ( YMode == NEW_NEWMV || YMode == NEAREST_NEWMV || YMode == NEAR_NEWMV )
             compMode = NEWMV
         else if ( YMode == NEAREST_NEARESTMV || YMode == NEW_NEARESTMV )
             compMode = NEARESTMV
         else if ( YMode == NEAR_NEARMV || YMode == NEW_NEARMV )
             compMode = NEARMV
         else
             compMode = GLOBALMV
     }
     return compMode
}



read_mv( ref ) {
     diffMv[ 0 ] = 0
     diffMv[ 1 ] = 0
     if ( use_intrabc ) {
         MvCtx = MV_INTRABC_CONTEXT
     } else {
         MvCtx = 0
     }
     S() mv_joint;
     if ( mv_joint == MV_JOINT_HZVNZ || mv_joint == MV_JOINT_HNZVNZ )
         diffMv[ 0 ] = read_mv_component( 0 )
     if ( mv_joint == MV_JOINT_HNZVZ || mv_joint == MV_JOINT_HNZVNZ )
         diffMv[ 1 ] = read_mv_component( 1 )
     Mv[ ref ][ 0 ] = PredMv[ ref ][ 0 ] + diffMv[ 0 ]
     Mv[ ref ][ 1 ] = PredMv[ ref ][ 1 ] + diffMv[ 1 ]
}



read_mv_component( comp ) {
     S() mv_sign;
     S() mv_class;
     if ( mv_class == MV_CLASS_0 ) {
         S() mv_class0_bit;
         if ( force_integer_mv )
             mv_class0_fr = 3
         else
             S() mv_class0_fr;
         if ( allow_high_precision_mv )
             S() mv_class0_hp;
         else
             mv_class0_hp = 1
         mag = ( ( mv_class0_bit << 3 ) |
                 ( mv_class0_fr << 1 ) |
                   mv_class0_hp ) + 1
     } else {
         d = 0
         for ( i = 0; i < mv_class; i++ ) {
             S() mv_bit;
             d |= mv_bit << i
         }
         mag = CLASS0_SIZE << ( mv_class + 2 )
         if ( force_integer_mv )
             mv_fr = 3
         else
             S() mv_fr;
         if ( allow_high_precision_mv )
             S() mv_hp;
         else
             mv_hp = 1
         mag += ( ( d << 3 ) | ( mv_fr << 1 ) | mv_hp ) + 1
     }
     return mv_sign ? -mag : mag
}



compute_prediction() {
     sbMask = use_128x128_superblock ? 31 : 15
     subBlockMiRow = MiRow & sbMask
     subBlockMiCol = MiCol & sbMask
     for ( plane = 0; plane < 1 + HasChroma * 2; plane++ ) {
         planeSz = get_plane_residual_size( MiSize, plane )
         num4x4W = Num_4x4_Blocks_Wide[ planeSz ]
         num4x4H = Num_4x4_Blocks_High[ planeSz ]
         log2W = MI_SIZE_LOG2 + Mi_Width_Log2[ planeSz ]
         log2H = MI_SIZE_LOG2 + Mi_Height_Log2[ planeSz ]
         subX = (plane > 0) ? subsampling_x : 0
         subY = (plane > 0) ? subsampling_y : 0
         baseX = (MiCol >> subX) * MI_SIZE
         baseY = (MiRow >> subY) * MI_SIZE
         candRow = (MiRow >> subY) << subY
         candCol = (MiCol >> subX) << subX

         IsInterIntra = ( is_inter && RefFrame[ 1 ] == INTRA_FRAME )
         if ( IsInterIntra ) {
             if ( interintra_mode == II_DC_PRED ) mode = DC_PRED
             else if ( interintra_mode == II_V_PRED ) mode = V_PRED
             else if ( interintra_mode == II_H_PRED ) mode = H_PRED
             else mode = SMOOTH_PRED
             predict_intra( plane, baseX, baseY,
                            plane == 0 ? AvailL : AvailLChroma,
                            plane == 0 ? AvailU : AvailUChroma,
                            BlockDecoded[ plane ]
                                        [ ( subBlockMiRow >> subY ) - 1 ]
                                        [ ( subBlockMiCol >> subX ) + num4x4W ],
                            BlockDecoded[ plane ]
                                        [ ( subBlockMiRow >> subY ) + num4x4H ]
                                        [ ( subBlockMiCol >> subX ) - 1 ],
                            mode,
                            log2W, log2H )
         }
         if ( is_inter ) {
             predW = Block_Width[ MiSize ] >> subX
             predH = Block_Height[ MiSize ] >> subY
             someUseIntra = 0
             for ( r = 0; r < (num4x4H << subY); r++ )
                 for ( c = 0; c < (num4x4W << subX); c++ )
                     if ( RefFrames[ candRow + r ][ candCol + c ][ 0 ] == INTRA_FRAME )
                         someUseIntra = 1
             if ( someUseIntra ) {
                 predW = num4x4W * 4
                 predH = num4x4H * 4
                 candRow = MiRow
                 candCol = MiCol
             }
             r = 0
             for ( y = 0; y < num4x4H * 4; y += predH ) {
                 c = 0
                 for ( x = 0; x < num4x4W * 4; x += predW ) {
                     predict_inter( plane, baseX + x, baseY + y,
                                    predW, predH,
                                    candRow + r, candCol + c)
                     c++
                 }
                 r++
             }
         }
     }
}



residual( ) {
     sbMask = use_128x128_superblock ? 31 : 15

     widthChunks = Max( 1, Block_Width[ MiSize ] >> 6 )
     heightChunks = Max( 1, Block_Height[ MiSize ] >> 6 )

     miSizeChunk = ( widthChunks > 1 || heightChunks > 1 ) ? BLOCK_64X64 : MiSize

     for ( chunkY = 0; chunkY < heightChunks; chunkY++ ) {
         for ( chunkX = 0; chunkX < widthChunks; chunkX++ ) {
             miRowChunk = MiRow + ( chunkY << 4 )
             miColChunk = MiCol + ( chunkX << 4 )
             subBlockMiRow = miRowChunk & sbMask
             subBlockMiCol = miColChunk & sbMask

             for ( plane = 0; plane < 1 + HasChroma * 2; plane++ ) {
                 txSz = Lossless ? TX_4X4 : get_tx_size( plane, TxSize )
                 stepX = Tx_Width[ txSz ] >> 2
                 stepY = Tx_Height[ txSz ] >> 2
                 planeSz = get_plane_residual_size( miSizeChunk, plane )
                 num4x4W = Num_4x4_Blocks_Wide[ planeSz ]
                 num4x4H = Num_4x4_Blocks_High[ planeSz ]
                 subX = (plane > 0) ? subsampling_x : 0
                 subY = (plane > 0) ? subsampling_y : 0
                 baseX = (miColChunk >> subX) * MI_SIZE
                 baseY = (miRowChunk >> subY) * MI_SIZE
                 if ( is_inter && !Lossless && !plane ) {
                     transform_tree( baseX, baseY, num4x4W * 4, num4x4H * 4 )
                 } else {
                     baseXBlock = (MiCol >> subX) * MI_SIZE
                     baseYBlock = (MiRow >> subY) * MI_SIZE
                     for ( y = 0; y < num4x4H; y += stepY )
                         for ( x = 0; x < num4x4W; x += stepX )
                             transform_block( plane, baseXBlock, baseYBlock, txSz,
                                              x + ( ( chunkX << 4 ) >> subX ),
                                              y + ( ( chunkY << 4 ) >> subY ) )
                 }
             }
         }
     }
}



transform_block(plane, baseX, baseY, txSz, x, y) {
     startX = baseX + 4 * x
     startY = baseY + 4 * y
     subX = (plane > 0) ? subsampling_x : 0
     subY = (plane > 0) ? subsampling_y : 0
     row = ( startY << subY ) >> MI_SIZE_LOG2
     col = ( startX << subX ) >> MI_SIZE_LOG2
     sbMask = use_128x128_superblock ? 31 : 15
     subBlockMiRow = row & sbMask
     subBlockMiCol = col & sbMask
     stepX = Tx_Width[ txSz ] >> MI_SIZE_LOG2
     stepY = Tx_Height[ txSz ] >> MI_SIZE_LOG2
     maxX = (MiCols * MI_SIZE) >> subX
     maxY = (MiRows * MI_SIZE) >> subY
     if ( startX >= maxX || startY >= maxY ) {
         return
     }
     if ( !is_inter ) {
         if ( ( ( plane == 0 ) && PaletteSizeY ) ||
              ( ( plane != 0 ) && PaletteSizeUV ) ) {
             predict_palette( plane, startX, startY, x, y, txSz )
         } else {
             isCfl = (plane > 0 && UVMode == UV_CFL_PRED)
             if ( plane == 0 ) {
                 mode = YMode
             } else {
                 mode = ( isCfl ) ? DC_PRED : UVMode
             }
             log2W = Tx_Width_Log2[ txSz ]
             log2H = Tx_Height_Log2[ txSz ]
             predict_intra( plane, startX, startY,
                            ( plane == 0 ? AvailL : AvailLChroma ) || x > 0,
                            ( plane == 0 ? AvailU : AvailUChroma ) || y > 0,
                            BlockDecoded[ plane ]
                                        [ ( subBlockMiRow >> subY ) - 1 ]
                                        [ ( subBlockMiCol >> subX ) + stepX ],
                            BlockDecoded[ plane ]
                                        [ ( subBlockMiRow >> subY ) + stepY ]
                                        [ ( subBlockMiCol >> subX ) - 1 ],
                            mode,
                            log2W, log2H )
             if ( isCfl ) {
                 predict_chroma_from_luma( plane, startX, startY, txSz )
             }
         }

         if ( plane == 0 ) {
             MaxLumaW = startX + stepX * 4
             MaxLumaH = startY + stepY * 4
         }
     }
     if ( !skip ) {
         eob = coeffs( plane, startX, startY, txSz )
         if ( eob > 0 )
             reconstruct( plane, startX, startY, txSz )
     }
     for ( i = 0; i < stepY; i++ ) {
         for ( j = 0; j < stepX; j++ ) {
             LoopfilterTxSizes[ plane ]
                              [ (row >> subY) + i ]
                              [ (col >> subX) + j ] = txSz
             BlockDecoded[ plane ]
                         [ ( subBlockMiRow >> subY ) + i ]
                         [ ( subBlockMiCol >> subX ) + j ] = 1
         }
     }
}



transform_tree( startX, startY, w, h ) {
     maxX = MiCols * MI_SIZE
     maxY = MiRows * MI_SIZE
     if ( startX >= maxX || startY >= maxY ) {
         return
     }
     row = startY >> MI_SIZE_LOG2
     col = startX >> MI_SIZE_LOG2
     lumaTxSz = InterTxSizes[ row ][ col ]
     lumaW = Tx_Width[ lumaTxSz ]
     lumaH = Tx_Height[ lumaTxSz ]
     if ( w <= lumaW && h <= lumaH ) {
         txSz = find_tx_size( w, h )
         transform_block( 0, startX, startY, txSz, 0, 0 )
     } else {
         if ( w > h ) {
             transform_tree( startX, startY, w/2, h )
             transform_tree( startX + w / 2, startY, w/2, h )
         } else if ( w < h ) {
             transform_tree( startX, startY, w, h/2 )
             transform_tree( startX, startY + h/2, w, h/2 )
         } else {
             transform_tree( startX, startY, w/2, h/2 )
             transform_tree( startX + w/2, startY, w/2, h/2 )
             transform_tree( startX, startY + h/2, w/2, h/2 )
             transform_tree( startX + w/2, startY + h/2, w/2, h/2 )
         }
     }
}



get_tx_size( plane, txSz ) {
     if ( plane == 0 )
         return txSz
     uvTx = Max_Tx_Size_Rect[ get_plane_residual_size( MiSize, plane ) ]
     if ( Tx_Width[ uvTx ] == 64 || Tx_Height[ uvTx ] == 64 ){
         if ( Tx_Width[ uvTx ] == 16 ) {
             return TX_16X32
         }
         if ( Tx_Height[ uvTx ] == 16 ) {
             return TX_32X16
         }
         return TX_32X32
     }
     return uvTx
}



get_plane_residual_size( subsize, plane ) {
     subx = plane > 0 ? subsampling_x : 0
     suby = plane > 0 ? subsampling_y : 0
     return Subsampled_Size[ subsize ][ subx ][ suby ]
}



coeffs( plane, startX, startY, txSz ) {
     x4 = startX >> 2
     y4 = startY >> 2
     w4 = Tx_Width[ txSz ] >> 2
     h4 = Tx_Height[ txSz ] >> 2

     txSzCtx = ( Tx_Size_Sqr[txSz] + Tx_Size_Sqr_Up[txSz] + 1 ) >> 1
     ptype = plane > 0
     segEob = ( txSz == TX_16X64 || txSz == TX_64X16 ) ? 512 :
                 Min( 1024, Tx_Width[ txSz ] * Tx_Height[ txSz ] )

     for ( c = 0; c < segEob; c++ )
         Quant[c] = 0
     for ( i = 0; i < 64; i++ )
         for ( j = 0; j < 64; j++ )
             Dequant[ i ][ j ] = 0

     eob = 0
     culLevel = 0
     dcCategory = 0

     S() all_zero;
     if ( all_zero ) {
         c = 0
         if ( plane == 0 ) {
             for ( i = 0; i < w4; i++ ) {
                 for ( j = 0; j < h4; j++ ) {
                     TxTypes[ y4 + j ][ x4 + i ] = DCT_DCT
                 }
             }
         }
     } else {
         if ( plane == 0 )
             transform_type( x4, y4, txSz )
         PlaneTxType = compute_tx_type( plane, txSz, x4, y4 )
         scan = get_scan( txSz )

         eobMultisize = Min( Tx_Width_Log2[ txSz ], 5) + Min( Tx_Height_Log2[ txSz ], 5) - 4
         if ( eobMultisize == 0 ) {
             S() eob_pt_16;
             eobPt = eob_pt_16 + 1
         } else if ( eobMultisize == 1 ) {
             S() eob_pt_32;
             eobPt = eob_pt_32 + 1
         } else if ( eobMultisize == 2 ) {
             S() eob_pt_64;
             eobPt = eob_pt_64 + 1
         } else if ( eobMultisize == 3 ) {
             S() eob_pt_128;
             eobPt = eob_pt_128 + 1
         } else if ( eobMultisize == 4 ) {
             S() eob_pt_256;
             eobPt = eob_pt_256 + 1
         } else if ( eobMultisize == 5 ) {
             S() eob_pt_512;
             eobPt = eob_pt_512 + 1
         } else {
             S() eob_pt_1024;
             eobPt = eob_pt_1024 + 1
         }

         eob = ( eobPt < 2 ) ? eobPt : ( ( 1 << ( eobPt - 2 ) ) + 1 )
         eobShift = Max( -1, eobPt - 3 )
         if ( eobShift >= 0 ) {
             S() eob_extra;
             if ( eob_extra ) {
                 eob += ( 1 << eobShift )
             }

             for ( i = 1; i < Max( 0, eobPt - 2 ); i++ ) {
                 eobShift = Max( 0, eobPt - 2 ) - 1 - i
                 L(1) eob_extra_bit;
                 if ( eob_extra_bit ) {
                     eob += ( 1 << eobShift )
                 }
             }
         }
         for ( c = eob - 1; c >= 0; c-- ) {
             pos = scan[ c ]
             if ( c == ( eob - 1 ) ) {
                 S() coeff_base_eob;
                 level = coeff_base_eob + 1
             } else {
                 S() coeff_base;
                 level = coeff_base
             }

             if ( level > NUM_BASE_LEVELS ) {
                 for ( idx = 0;
                       idx < COEFF_BASE_RANGE / ( BR_CDF_SIZE - 1 );
                       idx++ ) {
                     S() coeff_br;
                     level += coeff_br
                     if ( coeff_br < ( BR_CDF_SIZE - 1 ) )
                         break
                 }
             }
             Quant[ pos ] = level
         }

         for ( c = 0; c < eob; c++ ) {
             pos = scan[ c ]
             if ( Quant[ pos ] != 0 ) {
                 if ( c == 0 ) {
                     S() dc_sign;
                     sign = dc_sign
                 } else {
                     L(1) sign_bit;
                     sign = sign_bit
                 }
             } else {
                 sign = 0
             }
             if ( Quant[ pos ] >
                 ( NUM_BASE_LEVELS + COEFF_BASE_RANGE ) ) {
                 length = 0
                 do {
                     length++
                     L(1) golomb_length_bit;
                 } while ( !golomb_length_bit )
                 x = 1
                 for ( i = length - 2; i >= 0; i-- ) {
                     L(1) golomb_data_bit;
                     x = ( x << 1 ) | golomb_data_bit
                 }
                Quant[ pos ] = x + COEFF_BASE_RANGE + NUM_BASE_LEVELS
             }
             if ( pos == 0 && Quant[ pos ] > 0 ) {
                 dcCategory = sign ? 1 : 2
             }
             Quant[ pos ] = Quant[ pos ] & 0xFFFFF
             culLevel += Quant[ pos ]
             if ( sign )
                 Quant[ pos ] = - Quant[ pos ]
         }
         culLevel = Min( 63, culLevel )
     }

     for ( i = 0; i < w4; i++ ) {
         AboveLevelContext[ plane ][ x4 + i ] = culLevel
         AboveDcContext[ plane ][ x4 + i ] = dcCategory
     }
     for ( i = 0; i < h4; i++ ) {
         LeftLevelContext[ plane ][ y4 + i ] = culLevel
         LeftDcContext[ plane ][ y4 + i ] = dcCategory
     }

     return eob
}



compute_tx_type( plane, txSz, blockX, blockY ) {
     txSzSqrUp = Tx_Size_Sqr_Up[ txSz ]

     if ( Lossless || txSzSqrUp > TX_32X32 )
         return DCT_DCT

     txSet = get_tx_set( txSz )

     if ( plane == 0 ) {
         return TxTypes[ blockY ][ blockX ]
     }

     if ( is_inter ) {
         x4 = Max( MiCol, blockX << subsampling_x )
         y4 = Max( MiRow, blockY << subsampling_y )
         txType = TxTypes[ y4 ][ x4 ]
         if ( !is_tx_type_in_set( txSet, txType ) )
             return DCT_DCT
         return txType
     }

     txType = Mode_To_Txfm[ UVMode ]
     if ( !is_tx_type_in_set( txSet, txType ) )
         return DCT_DCT
     return txType
 }

 is_tx_type_in_set( txSet, txType ) {
     return is_inter ? Tx_Type_In_Set_Inter[ txSet ][ txType ] :
                       Tx_Type_In_Set_Intra[ txSet ][ txType ]
}



get_mrow_scan( txSz ) {
     if ( txSz == TX_4X4 )
         return Mrow_Scan_4x4
     else if ( txSz == TX_4X8 )
         return Mrow_Scan_4x8
     else if ( txSz == TX_8X4 )
         return Mrow_Scan_8x4
     else if ( txSz == TX_8X8 )
         return Mrow_Scan_8x8
     else if ( txSz == TX_8X16 )
         return Mrow_Scan_8x16
     else if ( txSz == TX_16X8 )
         return Mrow_Scan_16x8
     else if ( txSz == TX_16X16 )
         return Mrow_Scan_16x16
     else if ( txSz == TX_4X16 )
         return Mrow_Scan_4x16
     return Mrow_Scan_16x4
 }

 get_mcol_scan( txSz ) {
     if ( txSz == TX_4X4 )
         return Mcol_Scan_4x4
     else if ( txSz == TX_4X8 )
         return Mcol_Scan_4x8
     else if ( txSz == TX_8X4 )
         return Mcol_Scan_8x4
     else if ( txSz == TX_8X8 )
         return Mcol_Scan_8x8
     else if ( txSz == TX_8X16 )
         return Mcol_Scan_8x16
     else if ( txSz == TX_16X8 )
         return Mcol_Scan_16x8
     else if ( txSz == TX_16X16 )
         return Mcol_Scan_16x16
     else if ( txSz == TX_4X16 )
         return Mcol_Scan_4x16
     return Mcol_Scan_16x4
 }

 get_default_scan( txSz ) {
     if ( txSz == TX_4X4 )
         return Default_Scan_4x4
     else if ( txSz == TX_4X8 )
         return Default_Scan_4x8
     else if ( txSz == TX_8X4 )
         return Default_Scan_8x4
     else if ( txSz == TX_8X8 )
         return Default_Scan_8x8
     else if ( txSz == TX_8X16 )
         return Default_Scan_8x16
     else if ( txSz == TX_16X8 )
         return Default_Scan_16x8
     else if ( txSz == TX_16X16 )
         return Default_Scan_16x16
     else if ( txSz == TX_16X32 )
         return Default_Scan_16x32
     else if ( txSz == TX_32X16 )
         return Default_Scan_32x16
     else if ( txSz == TX_4X16 )
         return Default_Scan_4x16
     else if ( txSz == TX_16X4 )
         return Default_Scan_16x4
     else if ( txSz == TX_8X32 )
         return Default_Scan_8x32
     else if ( txSz == TX_32X8 )
         return Default_Scan_32x8
     return Default_Scan_32x32
 }

 get_scan( txSz ) {
     if ( txSz == TX_16X64 ) {
         return Default_Scan_16x32
     }
     if ( txSz == TX_64X16 ) {
         return Default_Scan_32x16
     }
     if ( Tx_Size_Sqr_Up[ txSz ] == TX_64X64 ) {
         return Default_Scan_32x32
     }

     if ( PlaneTxType == IDTX ) {
         return get_default_scan( txSz )
     }

     preferRow = ( PlaneTxType == V_DCT ||
                   PlaneTxType == V_ADST ||
                   PlaneTxType == V_FLIPADST )

     preferCol = ( PlaneTxType == H_DCT ||
                   PlaneTxType == H_ADST ||
                   PlaneTxType == H_FLIPADST )

     if ( preferRow ) {
         return get_mrow_scan( txSz )
     } else if ( preferCol ) {
         return get_mcol_scan( txSz )
     }
     return get_default_scan( txSz )
}



intra_angle_info_y( ) {
     AngleDeltaY = 0
     if ( MiSize >= BLOCK_8X8 ) {
         if ( is_directional_mode( YMode ) ) {
             S() angle_delta_y;
             AngleDeltaY = angle_delta_y - MAX_ANGLE_DELTA
         }
     }
}



intra_angle_info_uv( ) {
     AngleDeltaUV = 0
     if ( MiSize >= BLOCK_8X8 ) {
         if ( is_directional_mode( UVMode ) ) {
             S() angle_delta_uv;
             AngleDeltaUV = angle_delta_uv - MAX_ANGLE_DELTA
         }
     }
}



is_directional_mode( mode ) {
     if ( ( mode >= V_PRED ) && ( mode <= D67_PRED ) ) {
         return 1
     }
     return 0
}



read_cfl_alphas() {
     S() cfl_alpha_signs;
     signU = (cfl_alpha_signs + 1 ) / 3
     signV = (cfl_alpha_signs + 1 ) % 3
     if ( signU != CFL_SIGN_ZERO ) {
         S() cfl_alpha_u;
         CflAlphaU = 1 + cfl_alpha_u
         if ( signU == CFL_SIGN_NEG )
             CflAlphaU = -CflAlphaU
     } else {
       CflAlphaU = 0
     }
     if ( signV != CFL_SIGN_ZERO ) {
         S() cfl_alpha_v;
         CflAlphaV = 1 + cfl_alpha_v
         if ( signV == CFL_SIGN_NEG )
             CflAlphaV = -CflAlphaV
     } else {
       CflAlphaV = 0
    }



palette_mode_info( ) {
     bsizeCtx = Mi_Width_Log2[ MiSize ] + Mi_Height_Log2[ MiSize ] - 2
     if ( YMode == DC_PRED ) {
         S() has_palette_y;
         if ( has_palette_y ) {
             S() palette_size_y_minus_2;
             PaletteSizeY = palette_size_y_minus_2 + 2
             cacheN = get_palette_cache( 0 )
             idx = 0
             for ( i = 0; i < cacheN && idx < PaletteSizeY; i++ ) {
                 L(1) use_palette_color_cache_y;
                 if ( use_palette_color_cache_y ) {
                     palette_colors_y[ idx ] = PaletteCache[ i ]
                     idx++
                 }
             }
             if ( idx < PaletteSizeY ) {
                 L(BitDepth) palette_colors_y[ idx ];
                 idx++
             }
             if ( idx < PaletteSizeY ) {
                 minBits = BitDepth - 3
                 L(2) palette_num_extra_bits_y;
                 paletteBits = minBits + palette_num_extra_bits_y
             }
             while ( idx < PaletteSizeY ) {
                 L(paletteBits) palette_delta_y;
                 palette_delta_y++
                 palette_colors_y[ idx ] =
                           Clip1( palette_colors_y[ idx - 1 ] +
                                  palette_delta_y )
                 range = ( 1 << BitDepth ) - palette_colors_y[ idx ] - 1
                 paletteBits = Min( paletteBits, CeilLog2( range ) )
                 idx++
             }
             sort( palette_colors_y, 0, PaletteSizeY - 1 )
         }
     }
     if ( HasChroma && UVMode == DC_PRED ) {
         S() has_palette_uv;
         if ( has_palette_uv ) {
             S() palette_size_uv_minus_2;
             PaletteSizeUV = palette_size_uv_minus_2 + 2
             cacheN = get_palette_cache( 1 )
             idx = 0
             for ( i = 0; i < cacheN && idx < PaletteSizeUV; i++ ) {
                 L(1) use_palette_color_cache_u;
                 if ( use_palette_color_cache_u ) {
                     palette_colors_u[ idx ] = PaletteCache[ i ]
                     idx++
                 }
             }
             if ( idx < PaletteSizeUV ) {
                 L(BitDepth) palette_colors_u[ idx ];
                 idx++
             }
             if ( idx < PaletteSizeUV ) {
                 minBits = BitDepth - 3
                 L(2) palette_num_extra_bits_u;
                 paletteBits = minBits + palette_num_extra_bits_u
             }
             while ( idx < PaletteSizeUV ) {
                 L(paletteBits) palette_delta_u;
                 palette_colors_u[ idx ] =
                           Clip1( palette_colors_u[ idx - 1 ] +
                                  palette_delta_u )
                 range = ( 1 << BitDepth ) - palette_colors_u[ idx ]
                 paletteBits = Min( paletteBits, CeilLog2( range ) )
                 idx++
             }
             sort( palette_colors_u, 0, PaletteSizeUV - 1 )

             L(1) delta_encode_palette_colors_v;
             if ( delta_encode_palette_colors_v ) {
                 minBits = BitDepth - 4
                 maxVal = 1 << BitDepth
                 L(2) palette_num_extra_bits_v;
                 paletteBits = minBits + palette_num_extra_bits_v
                 L(BitDepth) palette_colors_v[ 0 ];
                 for ( idx = 1; idx < PaletteSizeUV; idx++ ) {
                     L(paletteBits) palette_delta_v;
                     if ( palette_delta_v ) {
                         L(1) palette_delta_sign_bit_v;
                         if ( palette_delta_sign_bit_v ) {
                             palette_delta_v = -palette_delta_v
                         }
                     }
                     val = palette_colors_v[ idx - 1 ] + palette_delta_v
                     if ( val < 0 ) val += maxVal
                     if ( val >= maxVal ) val -= maxVal
                     palette_colors_v[ idx ] = Clip1( val )
                 }
             } else {
                 for ( idx = 0; idx < PaletteSizeUV; idx++ ) {
                     L(BitDepth) palette_colors_v[ idx ];
                 }
             }
         }
     }
}



get_palette_cache( plane ) {
     aboveN = 0
     if ( ( MiRow * MI_SIZE ) % 64 ) {
         aboveN = PaletteSizes[ plane ][ MiRow - 1 ][ MiCol ]
     }
     leftN = 0
     if ( AvailL ) {
         leftN = PaletteSizes[ plane ][ MiRow ][ MiCol - 1 ]
     }
     aboveIdx = 0
     leftIdx = 0
     n = 0
     while ( aboveIdx < aboveN  && leftIdx < leftN ) {
         aboveC = PaletteColors[ plane ][ MiRow - 1 ][ MiCol ][ aboveIdx ]
         leftC = PaletteColors[ plane ][ MiRow ][ MiCol - 1 ][ leftIdx ]
         if ( leftC < aboveC ) {
             if ( n == 0 || leftC != PaletteCache[ n - 1 ] ) {
                 PaletteCache[ n ] = leftC
                 n++
             }
             leftIdx++
         } else {
             if ( n == 0 || aboveC != PaletteCache[ n - 1 ] ) {
                 PaletteCache[ n ] = aboveC
                 n++
             }
             aboveIdx++
             if ( leftC == aboveC ) {
                 leftIdx++
             }
         }
     }
     while ( aboveIdx < aboveN ) {
         val = PaletteColors[ plane ][ MiRow - 1 ][ MiCol ][ aboveIdx ]
         aboveIdx++
         if ( n == 0 || val != PaletteCache[ n - 1 ] ) {
             PaletteCache[ n ] = val
             n++
         }
     }
     while ( leftIdx < leftN ) {
         val = PaletteColors[ plane ][ MiRow ][ MiCol - 1 ][ leftIdx ]
         leftIdx++
         if ( n == 0 || val != PaletteCache[ n - 1 ] ) {
             PaletteCache[ n ] = val
             n++
         }
     }
     return n
}



transform_type( x4, y4, txSz ) {
     set = get_tx_set( txSz )

     if ( set > 0 &&
          ( segmentation_enabled ? get_qindex( 1, segment_id ) : base_q_idx ) > 0 ) {
         if ( is_inter ) {
             S() inter_tx_type;
             if ( set == TX_SET_INTER_1 )
                 TxType = Tx_Type_Inter_Inv_Set1[ inter_tx_type ]
             else if ( set == TX_SET_INTER_2 )
                 TxType = Tx_Type_Inter_Inv_Set2[ inter_tx_type ]
             else
                 TxType = Tx_Type_Inter_Inv_Set3[ inter_tx_type ]
         } else {
             S() intra_tx_type;
             if ( set == TX_SET_INTRA_1 )
                 TxType = Tx_Type_Intra_Inv_Set1[ intra_tx_type ]
             else
                 TxType = Tx_Type_Intra_Inv_Set2[ intra_tx_type ]
         }
     } else {
         TxType = DCT_DCT
     }
     for ( i = 0; i < ( Tx_Width[ txSz ] >> 2 ); i++ ) {
         for ( j = 0; j < ( Tx_Height[ txSz ] >> 2 ); j++ ) {
             TxTypes[ y4 + j ][ x4 + i ] = TxType
         }
     }
}



get_tx_set( txSz ) {
     txSzSqr = Tx_Size_Sqr[ txSz ]
     txSzSqrUp = Tx_Size_Sqr_Up[ txSz ]
     if ( txSzSqrUp > TX_32X32 )
         return TX_SET_DCTONLY
     if ( is_inter ) {
         if ( reduced_tx_set || txSzSqrUp == TX_32X32 ) return TX_SET_INTER_3
         else if ( txSzSqr == TX_16X16 ) return TX_SET_INTER_2
         return TX_SET_INTER_1
     } else {
         if ( txSzSqrUp == TX_32X32 ) return TX_SET_DCTONLY
         else if ( reduced_tx_set ) return TX_SET_INTRA_2
         else if ( txSzSqr == TX_16X16 ) return TX_SET_INTRA_2
         return TX_SET_INTRA_1
     }
}



palette_tokens( ) {
     blockHeight = Block_Height[ MiSize ]
     blockWidth = Block_Width[ MiSize ]
     onscreenHeight = Min( blockHeight, (MiRows - MiRow) * MI_SIZE )
     onscreenWidth = Min( blockWidth, (MiCols - MiCol) * MI_SIZE )

     if ( PaletteSizeY ) {
         NS(PaletteSizeY) color_index_map_y;
         ColorMapY[0][0] = color_index_map_y
         for ( i = 1; i < onscreenHeight + onscreenWidth - 1; i++ ) {
             for ( j = Min( i, onscreenWidth - 1 );
                       j >= Max( 0, i - onscreenHeight + 1 ); j-- ) {
                 get_palette_color_context(
                     ColorMapY, ( i - j ), j, PaletteSizeY )
                 S() palette_color_idx_y;
                 ColorMapY[ i - j ][ j ] = ColorOrder[ palette_color_idx_y ]
             }
         }
         for ( i = 0; i < onscreenHeight; i++ ) {
             for ( j = onscreenWidth; j < blockWidth; j++ ) {
                 ColorMapY[ i ][ j ] = ColorMapY[ i ][ onscreenWidth - 1 ]
             }
         }
         for ( i = onscreenHeight; i < blockHeight; i++ ) {
             for ( j = 0; j < blockWidth; j++ ) {
                 ColorMapY[ i ][ j ] = ColorMapY[ onscreenHeight - 1 ][ j ]
             }
         }
     }

     if ( PaletteSizeUV ) {
         NS(PaletteSizeUV) color_index_map_uv;
         ColorMapUV[0][0] = color_index_map_uv
         blockHeight = blockHeight >> subsampling_y
         blockWidth = blockWidth >> subsampling_x
         onscreenHeight = onscreenHeight >> subsampling_y
         onscreenWidth = onscreenWidth >> subsampling_x
         if ( blockWidth < 4 ) {
             blockWidth += 2
             onscreenWidth += 2
         }
         if ( blockHeight < 4 ) {
             blockHeight += 2
             onscreenHeight += 2
         }

         for ( i = 1; i < onscreenHeight + onscreenWidth - 1; i++ ) {
             for ( j = Min( i, onscreenWidth - 1 );
                       j >= Max( 0, i - onscreenHeight + 1 ); j-- ) {
                 get_palette_color_context(
                     ColorMapUV, ( i - j ), j, PaletteSizeUV )
                 S() palette_color_idx_uv;
                 ColorMapUV[ i - j ][ j ] = ColorOrder[ palette_color_idx_uv ]
             }
         }
         for ( i = 0; i < onscreenHeight; i++ ) {
             for ( j = onscreenWidth; j < blockWidth; j++ ) {
                 ColorMapUV[ i ][ j ] = ColorMapUV[ i ][ onscreenWidth - 1 ]
             }
         }
         for ( i = onscreenHeight; i < blockHeight; i++ ) {
             for ( j = 0; j < blockWidth; j++ ) {
                 ColorMapUV[ i ][ j ] = ColorMapUV[ onscreenHeight - 1 ][ j ]
             }
         }
     }
}



get_palette_color_context( colorMap, r, c, n ) {
     for ( i = 0; i < PALETTE_COLORS; i++ ) {
         scores[ i ] = 0
         ColorOrder[i] = i
     }
     if ( c > 0 ) {
         neighbor = colorMap[ r ][ c - 1 ]
         scores[ neighbor ] += 2
     }
     if ( ( r > 0 ) && ( c > 0 ) ) {
         neighbor = colorMap[ r - 1 ][ c - 1 ]
         scores[ neighbor ] += 1
     }
     if ( r > 0 ) {
         neighbor = colorMap[ r - 1 ][ c ]
         scores[ neighbor ] += 2
     }
     for ( i = 0; i < PALETTE_NUM_NEIGHBORS; i++ ) {
         maxScore = scores[ i ]
         maxIdx = i
         for ( j = i + 1; j < n; j++ ) {
             if ( scores[ j ] > maxScore ) {
                 maxScore = scores[ j ]
                 maxIdx = j
             }
         }
         if ( maxIdx != i ) {
             maxScore = scores[ maxIdx ]
             maxColorOrder = ColorOrder[ maxIdx ]
             for ( k = maxIdx; k > i; k-- ) {
                 scores[ k ] = scores[ k - 1 ]
                 ColorOrder[ k ] = ColorOrder[ k - 1 ]
             }
             scores[ i ] = maxScore
             ColorOrder[ i ] = maxColorOrder
         }
     }
     ColorContextHash = 0
     for ( i = 0; i < PALETTE_NUM_NEIGHBORS; i++ ) {
         ColorContextHash += scores[ i ] * Palette_Color_Hash_Multipliers[ i ]
     }
}



is_inside( candidateR, candidateC ) {
     return ( candidateC >= MiColStart &&
              candidateC < MiColEnd &&
              candidateR >= MiRowStart &&
              candidateR < MiRowEnd )
}



is_inside_filter_region( candidateR, candidateC ) {
     colStart = 0
     colEnd = MiCols
     rowStart = 0
     rowEnd = MiRows
     return (candidateC >= colStart &&
             candidateC < colEnd &&
             candidateR >= rowStart &&
             candidateR < rowEnd)
}



clamp_mv_row( mvec, border ) {
     bh4 = Num_4x4_Blocks_High[ MiSize ]
     mbToTopEdge = -((MiRow * MI_SIZE) * 8)
     mbToBottomEdge = ((MiRows - bh4 - MiRow) * MI_SIZE) * 8
     return Clip3( mbToTopEdge - border, mbToBottomEdge + border, mvec )
}



clamp_mv_col( mvec, border ) {
     bw4 = Num_4x4_Blocks_Wide[ MiSize ]
     mbToLeftEdge = -((MiCol * MI_SIZE) * 8)
     mbToRightEdge = ((MiCols - bw4 - MiCol) * MI_SIZE) * 8
     return Clip3( mbToLeftEdge - border, mbToRightEdge + border, mvec )
}



clear_cdef( r, c ) {
     cdef_idx[ r ][ c ] = -1
     if ( use_128x128_superblock ) {
         cdefSize4 = Num_4x4_Blocks_Wide[ BLOCK_64X64 ]
         cdef_idx[ r ][ c + cdefSize4 ] = -1
         cdef_idx[ r + cdefSize4][ c ] = -1
         cdef_idx[ r + cdefSize4][ c + cdefSize4 ] = -1
     }
}



read_cdef( ) {
     if ( skip || CodedLossless || !enable_cdef || allow_intrabc) {
         return
     }
     cdefSize4 = Num_4x4_Blocks_Wide[ BLOCK_64X64 ]
     cdefMask4 = ~(cdefSize4 - 1)
     r = MiRow & cdefMask4
     c = MiCol & cdefMask4
     if ( cdef_idx[ r ][ c ] == -1 ) {
         L(cdef_bits) cdef_idx[ r ][ c ];
         w4 = Num_4x4_Blocks_Wide[ MiSize ]
         h4 = Num_4x4_Blocks_High[ MiSize ]
         for ( i = r; i < r + h4 ; i += cdefSize4 ) {
             for ( j = c; j < c + w4 ; j += cdefSize4 ) {
                 cdef_idx[ i ][ j ] = cdef_idx[ r ][ c ]
             }
         }
     }
}



read_lr( r, c, bSize ) {
     if ( allow_intrabc ) {
         return
     }
     w = Num_4x4_Blocks_Wide[ bSize ]
     h = Num_4x4_Blocks_High[ bSize ]
     for ( plane = 0; plane < NumPlanes; plane++ ) {
         if ( FrameRestorationType[ plane ] != RESTORE_NONE ) {
             subX = (plane == 0) ? 0 : subsampling_x
             subY = (plane == 0) ? 0 : subsampling_y
             unitSize = LoopRestorationSize[ plane ]
             unitRows = count_units_in_frame( unitSize, Round2( FrameHeight, subY) )
             unitCols = count_units_in_frame( unitSize, Round2( UpscaledWidth, subX) )
             unitRowStart = ( r * ( MI_SIZE >> subY) +
                                       unitSize - 1 ) / unitSize
             unitRowEnd = Min( unitRows, ( (r + h) * ( MI_SIZE >> subY) +
                                       unitSize - 1 ) / unitSize)
             if ( use_superres ) {
                 numerator = (MI_SIZE >> subX) * SuperresDenom
                 denominator = unitSize * SUPERRES_NUM
             } else {
                 numerator = MI_SIZE >> subX
                 denominator = unitSize
             }
             unitColStart = ( c * numerator + denominator - 1 ) / denominator
             unitColEnd = Min( unitCols, ( (c + w) * numerator +
                               denominator - 1 ) / denominator)
             for ( unitRow = unitRowStart; unitRow < unitRowEnd; unitRow++ ) {
                 for ( unitCol = unitColStart; unitCol < unitColEnd; unitCol++ ) {
                     read_lr_unit(plane, unitRow, unitCol)
                 }
             }
         }
     }
}



read_lr_unit(plane, unitRow, unitCol) {
     if ( FrameRestorationType[ plane ] == RESTORE_WIENER ) {
         S() use_wiener;
         restoration_type = use_wiener ? RESTORE_WIENER : RESTORE_NONE
     } else if ( FrameRestorationType[ plane ] == RESTORE_SGRPROJ ) {
         S() use_sgrproj;
         restoration_type = use_sgrproj ? RESTORE_SGRPROJ : RESTORE_NONE
     } else {
         S() restoration_type;
     }
     LrType[ plane ][ unitRow ][ unitCol ] = restoration_type
     if ( restoration_type == RESTORE_WIENER ) {
         for ( pass = 0; pass < 2; pass++ ) {
             if ( plane ) {
                 firstCoeff = 1
                 LrWiener[ plane ]
                         [ unitRow ][ unitCol ][ pass ][0] = 0
             } else {
                 firstCoeff = 0
             }
             for ( j = firstCoeff; j < 3; j++ ) {
                 min = Wiener_Taps_Min[ j ]
                 max = Wiener_Taps_Max[ j ]
                 k = Wiener_Taps_K[ j ]
                 v = decode_signed_subexp_with_ref_bool(
                         min, max + 1, k, RefLrWiener[ plane ][ pass ][ j ] )
                 LrWiener[ plane ]
                         [ unitRow ][ unitCol ][ pass ][ j ] = v
                 RefLrWiener[ plane ][ pass ][ j ] = v
             }
         }
     } else if ( restoration_type == RESTORE_SGRPROJ ) {
         L(SGRPROJ_PARAMS_BITS) lr_sgr_set;
         LrSgrSet[ plane ][ unitRow ][ unitCol ] = lr_sgr_set
         for ( i = 0; i < 2; i++ ) {
             radius = Sgr_Params[ lr_sgr_set ][ i * 2 ]
             min = Sgrproj_Xqd_Min[i]
             max = Sgrproj_Xqd_Max[i]
             if ( radius ) {
               v = decode_signed_subexp_with_ref_bool(
                      min, max + 1, SGRPROJ_PRJ_SUBEXP_K,
                      RefSgrXqd[ plane ][ i ])
             } else {
               v = 0
               if ( i == 1 ) {
                 v = Clip3( min, max, (1 << SGRPROJ_PRJ_BITS) -
                            RefSgrXqd[ plane ][ 0 ] )
               }
             }
             LrSgrXqd[ plane ][ unitRow ][ unitCol ][ i ] = v
             RefSgrXqd[ plane ][ i ] = v
         }
     }
}



decode_signed_subexp_with_ref_bool( low, high, k, r ) {
     x = decode_unsigned_subexp_with_ref_bool(high - low, k, r - low)
     return x + low
 }

 decode_unsigned_subexp_with_ref_bool( mx, k, r ) {
     v = decode_subexp_bool( mx, k )
     if ( (r << 1) <= mx ) {
         return inverse_recenter(r, v)
     } else {
         return mx - 1 - inverse_recenter(mx - 1 - r, v)
     }
 }

 decode_subexp_bool( numSyms, k ) {
     i = 0
     mk = 0
     while ( 1 ) {
         b2 = i ? k + i - 1 : k
         a = 1 << b2
         if ( numSyms <= mk + 3 * a ) {
             NS(numSyms - mk) subexp_unif_bools;
             return subexp_unif_bools + mk
         } else {
             L(1) subexp_more_bools;
             if ( subexp_more_bools ) {
                i++
                mk += a
             } else {
                L(b2) subexp_bools;
                return subexp_bools + mk
             }
         }
     }
}



tile_list_obu( ) {
     f(8) output_frame_width_in_tiles_minus_1;
     f(8) output_frame_height_in_tiles_minus_1;
     f(16) tile_count_minus_1;
     for ( tile = 0; tile <= tile_count_minus_1; tile++ )
         tile_list_entry( )
}



tile_list_entry( ) {
     f(8) anchor_frame_idx;
     f(8) anchor_tile_row;
     f(8) anchor_tile_col;
     f(16) tile_data_size_minus_1;
     N = 8 * (tile_data_size_minus_1 + 1)
     f(N) coded_tile_data;
}



