void Sphere::intersect(const RenderContext& context, RayPacket& rays) const
{
  if(rays.getAllFlags() & RayPacket::NormalizedDirections){
    // RayPacket::NormalizedDirections
    if(rays.getAllFlags() & RayPacket::ConstantOrigin){
      // RayPacket::ConstantOrigin
      // Special case: constant origin, unit directions

      // Begin loop invariant setup code
      Vector rtsl_O = (rays.getOrigin(0) - center);
      Real rtsl_A = 1.f;
      Real rtsl_A_inv = (1 / rtsl_A);
      Real rtsl_C = (Dot(rtsl_O, rtsl_O) - (radius * radius));
      Real rtsl_invariant0 = (radius * radius);
      // End loop invariant setup

      int debugFlag = rays.getAllFlags()&RayPacket::DebugPacket;
      if (debugFlag) {
        cerr << "Sphere::intersect called" << endl;
      }

#ifdef MANTA_SSE
      RayPacketData* data = rays.data;
      if((rays.rayBegin ^ (rays.rayEnd-1)) & ~3){
        int i = rays.rayBegin & ~3;
        // Prologue
        if(i != rays.rayBegin){
          __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
          // mask is on for active rays
          __m128 mask = _mm_castsi128_ps(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)));
          int intmask = _mm_movemask_ps(mask);
          // Section 0: BEGIN Loading variables from previous sections

          __m128 rtsl_O_x = _mm_set1_ps(rtsl_O.x());
          __m128 rtsl_O_y = _mm_set1_ps(rtsl_O.y());
          __m128 rtsl_O_z = _mm_set1_ps(rtsl_O.z());
          // Section 0:   END Loading variables from previous sections

          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

          // scope for D: 5
          __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
          __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
          __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
          // Current scope = 5, scope for O = -1
          // Current scope = 5, scope for D = 5
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // scope for disc: 5
          __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_set1_ps(rtsl_C));
          // Current scope = 5, scope for disc = 5
          __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
          __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
          int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
          if(ifintmask4_section_undefined != 0){
            // Current scope = 6, scope for B = 5
            // scope for t: 6
            __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), _mm_set1_ps(rtsl_A_inv));
            // Current scope = 6, scope for D = 5
            // Current scope = 6, scope for t = 6
            // Current scope = 6, scope for O = -1
            rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
            rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
            rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
            // Current scope = 6, scope for O = -1
            // Current scope = 6, scope for D = 5
            // scope for B: 6
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 6, scope for O = -1
            // Current scope = 6, scope for O = -1
            // scope for C: 6
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for C = 6
            // Current scope = 6, scope for disc = 5
            rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_)));
            // Current scope = 6, scope for disc = 5
            __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
            __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
            int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
            if(ifintmask8_section_undefined != 0){
              // Current scope = 7, scope for disc = 5
              // scope for r: 7
              __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
              // Current scope = 7, scope for t = 6
              // Current scope = 7, scope for r = 7
              // Current scope = 7, scope for B = 6
              // scope for t1: 7
              __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)));
              // Current scope = 7, scope for t1 = 7
              __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
              int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
              __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
              int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
              if(ifintmask12_section_undefined != 0){
                // Current scope = 8, scope for r = 7
                // Current scope = 8, scope for B = 6
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)))));
              }
              __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
              int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
              if(elseintmask14_section_undefined != 0){
                // Current scope = 8, scope for t1 = 7
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
              }
              // Early termination
              if (intmask == 0) {
                goto prologue_next_ray0;
              }
              // Update the current mask
              ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
              ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
              // Current scope = 7, scope for t = 6
              rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
            }
            // Early termination
            if (intmask == 0) {
              goto prologue_next_ray0;
            }
            // Update the current mask
            ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
            ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
          }
          // Early termination
          if (intmask == 0) {
            goto prologue_next_ray0;
          }
          // Update the current mask
          mask = _mm_and_ps(mask, mask);
          intmask = intmask & intmask;
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

          i+=4;
        }
 prologue_next_ray0:;
        // Primary loop body
        int e = rays.rayEnd - 3;
        for(;i<e;i+=4){
          {
            // mask is on for active rays
            __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0xffffffff));
            int intmask = 15;
            // Section 0: BEGIN Loading variables from previous sections

            __m128 rtsl_O_x = _mm_set1_ps(rtsl_O.x());
            __m128 rtsl_O_y = _mm_set1_ps(rtsl_O.y());
            __m128 rtsl_O_z = _mm_set1_ps(rtsl_O.z());
            // Section 0:   END Loading variables from previous sections

            // Section 0: BEGIN Declaring variables that were written but not read yet

            // Section 0:   END Declaring variables that were written but not read yet

            // scope for D: 5
            __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
            __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
            __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
            // Current scope = 5, scope for O = -1
            // Current scope = 5, scope for D = 5
            // scope for B: 5
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 5, scope for B = 5
            // Current scope = 5, scope for B = 5
            // scope for disc: 5
            __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_set1_ps(rtsl_C));
            // Current scope = 5, scope for disc = 5
            __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
            __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
            int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
            if(ifintmask4_section_undefined != 0){
              // Current scope = 6, scope for B = 5
              // scope for t: 6
              __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), _mm_set1_ps(rtsl_A_inv));
              // Current scope = 6, scope for D = 5
              // Current scope = 6, scope for t = 6
              // Current scope = 6, scope for O = -1
              rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
              rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
              rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
              // Current scope = 6, scope for O = -1
              // Current scope = 6, scope for D = 5
              // scope for B: 6
              __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
              // Current scope = 6, scope for O = -1
              // Current scope = 6, scope for O = -1
              // scope for C: 6
              __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
              // Current scope = 6, scope for B = 6
              // Current scope = 6, scope for B = 6
              // Current scope = 6, scope for C = 6
              // Current scope = 6, scope for disc = 5
              rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_)));
              // Current scope = 6, scope for disc = 5
              __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
              //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
              __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
              int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
              if(ifintmask8_section_undefined != 0){
                // Current scope = 7, scope for disc = 5
                // scope for r: 7
                __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
                // Current scope = 7, scope for t = 6
                // Current scope = 7, scope for r = 7
                // Current scope = 7, scope for B = 6
                // scope for t1: 7
                __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)));
                // Current scope = 7, scope for t1 = 7
                __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
                int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
                __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
                int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
                if(ifintmask12_section_undefined != 0){
                  // Current scope = 8, scope for r = 7
                  // Current scope = 8, scope for B = 6
                  // Current scope = 8, scope for t = 6
                  rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)))));
                }
                __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
                int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
                if(elseintmask14_section_undefined != 0){
                  // Current scope = 8, scope for t1 = 7
                  // Current scope = 8, scope for t = 6
                  rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
                }
                // Early termination
                if (intmask == 0) {
                  goto for_loop_end1;
                }
                // Update the current mask
                ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
                ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
                // Current scope = 7, scope for t = 6
                rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
              }
              // Early termination
              if (intmask == 0) {
                goto for_loop_end1;
              }
              // Update the current mask
              ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
              ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
            }
            // Early termination
            if (intmask == 0) {
              goto for_loop_end1;
            }
            // Update the current mask
            mask = _mm_and_ps(mask, mask);
            intmask = intmask & intmask;
            // Section 0: BEGIN Storing variables people will need later.

            // Section 0:   END Storing variables people will need later.

            }
 for_loop_end1:;
        }
        // Epilogue
        if(i != rays.rayEnd){
          __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
          // mask is on for active rays
          __m128 mask = _mm_castsi128_ps(_mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i)));
          int intmask = _mm_movemask_ps(mask);
          // Section 0: BEGIN Loading variables from previous sections

          __m128 rtsl_O_x = _mm_set1_ps(rtsl_O.x());
          __m128 rtsl_O_y = _mm_set1_ps(rtsl_O.y());
          __m128 rtsl_O_z = _mm_set1_ps(rtsl_O.z());
          // Section 0:   END Loading variables from previous sections

          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

          // scope for D: 5
          __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
          __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
          __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
          // Current scope = 5, scope for O = -1
          // Current scope = 5, scope for D = 5
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // scope for disc: 5
          __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_set1_ps(rtsl_C));
          // Current scope = 5, scope for disc = 5
          __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
          __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
          int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
          if(ifintmask4_section_undefined != 0){
            // Current scope = 6, scope for B = 5
            // scope for t: 6
            __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), _mm_set1_ps(rtsl_A_inv));
            // Current scope = 6, scope for D = 5
            // Current scope = 6, scope for t = 6
            // Current scope = 6, scope for O = -1
            rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
            rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
            rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
            // Current scope = 6, scope for O = -1
            // Current scope = 6, scope for D = 5
            // scope for B: 6
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 6, scope for O = -1
            // Current scope = 6, scope for O = -1
            // scope for C: 6
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for C = 6
            // Current scope = 6, scope for disc = 5
            rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_)));
            // Current scope = 6, scope for disc = 5
            __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
            __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
            int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
            if(ifintmask8_section_undefined != 0){
              // Current scope = 7, scope for disc = 5
              // scope for r: 7
              __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
              // Current scope = 7, scope for t = 6
              // Current scope = 7, scope for r = 7
              // Current scope = 7, scope for B = 6
              // scope for t1: 7
              __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)));
              // Current scope = 7, scope for t1 = 7
              __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
              int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
              __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
              int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
              if(ifintmask12_section_undefined != 0){
                // Current scope = 8, scope for r = 7
                // Current scope = 8, scope for B = 6
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)))));
              }
              __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
              int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
              if(elseintmask14_section_undefined != 0){
                // Current scope = 8, scope for t1 = 7
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
              }
              // Early termination
              if (intmask == 0) {
                goto epilogue_next_ray2;
              }
              // Update the current mask
              ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
              ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
              // Current scope = 7, scope for t = 6
              rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
            }
            // Early termination
            if (intmask == 0) {
              goto epilogue_next_ray2;
            }
            // Update the current mask
            ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
            ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
          }
          // Early termination
          if (intmask == 0) {
            goto epilogue_next_ray2;
          }
          // Update the current mask
          mask = _mm_and_ps(mask, mask);
          intmask = intmask & intmask;
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

        }
 epilogue_next_ray2:;
      } else {
      // Single SSE vector
        int i = rays.rayBegin & ~3;
        __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
        __m128 mask = _mm_castsi128_ps(_mm_and_si128(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)), _mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i))));
        int intmask = _mm_movemask_ps(mask);
        // Section 0: BEGIN Loading variables from previous sections

        __m128 rtsl_O_x = _mm_set1_ps(rtsl_O.x());
        __m128 rtsl_O_y = _mm_set1_ps(rtsl_O.y());
        __m128 rtsl_O_z = _mm_set1_ps(rtsl_O.z());
        // Section 0:   END Loading variables from previous sections

        // Section 0: BEGIN Declaring variables that were written but not read yet

        // Section 0:   END Declaring variables that were written but not read yet

        // scope for D: 4
        __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
        __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
        __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
        // Current scope = 4, scope for O = -1
        // Current scope = 4, scope for D = 4
        // scope for B: 4
        __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
        // Current scope = 4, scope for B = 4
        // Current scope = 4, scope for B = 4
        // scope for disc: 4
        __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_set1_ps(rtsl_C));
        // Current scope = 4, scope for disc = 4
        __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
        //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
        __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
        int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
        if(ifintmask4_section_undefined != 0){
          // Current scope = 5, scope for B = 4
          // scope for t: 5
          __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), _mm_set1_ps(rtsl_A_inv));
          // Current scope = 5, scope for D = 4
          // Current scope = 5, scope for t = 5
          // Current scope = 5, scope for O = -1
          rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
          rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
          rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
          // Current scope = 5, scope for O = -1
          // Current scope = 5, scope for D = 4
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for O = -1
          // Current scope = 5, scope for O = -1
          // scope for C: 5
          __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for C = 5
          // Current scope = 5, scope for disc = 4
          rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_)));
          // Current scope = 5, scope for disc = 4
          __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
          __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
          int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
          if(ifintmask8_section_undefined != 0){
            // Current scope = 6, scope for disc = 4
            // scope for r: 6
            __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
            // Current scope = 6, scope for t = 5
            // Current scope = 6, scope for r = 6
            // Current scope = 6, scope for B = 5
            // scope for t1: 6
            __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)));
            // Current scope = 6, scope for t1 = 6
            __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
            int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
            __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
            int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
            if(ifintmask12_section_undefined != 0){
              // Current scope = 7, scope for r = 6
              // Current scope = 7, scope for B = 5
              // Current scope = 7, scope for t = 5
              rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)))));
            }
            __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
            int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
            if(elseintmask14_section_undefined != 0){
              // Current scope = 7, scope for t1 = 6
              // Current scope = 7, scope for t = 5
              rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
            }
            // Early termination
            if (intmask == 0) {
              goto single_vector_next_ray3;
            }
            // Update the current mask
            ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
            ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
            // Current scope = 6, scope for t = 5
            rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
          }
          // Early termination
          if (intmask == 0) {
            goto single_vector_next_ray3;
          }
          // Update the current mask
          ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
          ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
        }
        // Early termination
        if (intmask == 0) {
          goto single_vector_next_ray3;
        }
        // Update the current mask
        mask = _mm_and_ps(mask, mask);
        intmask = intmask & intmask;
        // Section 0: BEGIN Storing variables people will need later.

        // Section 0:   END Storing variables people will need later.

      }
 single_vector_next_ray3:;
#else
      for(int i = rays.begin(); i < rays.end(); i++){
        {
          // Section 0: BEGIN Loading variables from previous sections

          Vector rtsl_O(rays.getOrigin(i)-center);
          // Section 0:   END Loading variables from previous sections

          // Section 0: BEGIN Declaring variables that were written but not read yet

           // No declaration for O
          // Section 0:   END Declaring variables that were written but not read yet

            Vector rtsl_D = rays.getDirection(i);
            Real rtsl_B = Dot(rtsl_O, rtsl_D);
            Real rtsl_disc = ((rtsl_B * rtsl_B) - rtsl_C);
            if((rtsl_disc >= 0.f)){
              Real rtsl_t = ((-(rtsl_B)) * rtsl_A_inv);
              rtsl_O += (rtsl_D * rtsl_t);
              Real rtsl_B = Dot(rtsl_O, rtsl_D);
              Real rtsl_C = (Dot(rtsl_O, rtsl_O) - rtsl_invariant0);
              rtsl_disc = ((rtsl_B * rtsl_B) - rtsl_C);
              if((rtsl_disc >= 0.f)){
                Real rtsl_r = Sqrt(rtsl_disc);
                Real rtsl_t1 = (rtsl_t - ((rtsl_r + rtsl_B) * rtsl_A_inv));
                if((rtsl_t1 <= T_EPSILON)){
                  rtsl_t += ((rtsl_r - rtsl_B) * rtsl_A_inv);
                } else {
                  rtsl_t = rtsl_t1;
                }
                rays.hit(i, rtsl_t, getMaterial(), this, getTexCoordMapper());
              }
            }
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

          }
        }
#endif
    } else {
      // !RayPacket::ConstantOrigin
      // Special case: unit directions

      // Begin loop invariant setup code
      Real rtsl_A = 1.f;
      Real rtsl_A_inv = (1 / rtsl_A);
      Real rtsl_invariant0 = (radius * radius);
      Real rtsl_invariant1 = (radius * radius);
      // End loop invariant setup

      int debugFlag = rays.getAllFlags()&RayPacket::DebugPacket;
      if (debugFlag) {
        cerr << "Sphere::intersect called" << endl;
      }

#ifdef MANTA_SSE
      RayPacketData* data = rays.data;
      if((rays.rayBegin ^ (rays.rayEnd-1)) & ~3){
        int i = rays.rayBegin & ~3;
        // Prologue
        if(i != rays.rayBegin){
          __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
          // mask is on for active rays
          __m128 mask = _mm_castsi128_ps(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)));
          int intmask = _mm_movemask_ps(mask);
          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

          // scope for O: 5
          __m128 rtsl_O_x = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[0][i])), _mm_set1_ps(center.x()));
          __m128 rtsl_O_y = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[1][i])), _mm_set1_ps(center.y()));
          __m128 rtsl_O_z = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[2][i])), _mm_set1_ps(center.z()));
          // scope for D: 5
          __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
          __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
          __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
          // Current scope = 5, scope for O = 5
          // Current scope = 5, scope for D = 5
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for O = 5
          // Current scope = 5, scope for O = 5
          // scope for C: 5
          __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for C = 5
          // scope for disc: 5
          __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_);
          // Current scope = 5, scope for disc = 5
          __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
          __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
          int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
          if(ifintmask4_section_undefined != 0){
            // Current scope = 6, scope for B = 5
            // scope for t: 6
            __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), _mm_set1_ps(rtsl_A_inv));
            // Current scope = 6, scope for D = 5
            // Current scope = 6, scope for t = 6
            // Current scope = 6, scope for O = 5
            rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
            rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
            rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
            // Current scope = 6, scope for O = 5
            // Current scope = 6, scope for D = 5
            // scope for B: 6
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 6, scope for O = 5
            // Current scope = 6, scope for O = 5
            // scope for C: 6
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant1));
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for C = 6
            // Current scope = 6, scope for disc = 5
            rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_)));
            // Current scope = 6, scope for disc = 5
            __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
            __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
            int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
            if(ifintmask8_section_undefined != 0){
              // Current scope = 7, scope for disc = 5
              // scope for r: 7
              __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
              // Current scope = 7, scope for t = 6
              // Current scope = 7, scope for r = 7
              // Current scope = 7, scope for B = 6
              // scope for t1: 7
              __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)));
              // Current scope = 7, scope for t1 = 7
              __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
              int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
              __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
              int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
              if(ifintmask12_section_undefined != 0){
                // Current scope = 8, scope for r = 7
                // Current scope = 8, scope for B = 6
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)))));
              }
              __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
              int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
              if(elseintmask14_section_undefined != 0){
                // Current scope = 8, scope for t1 = 7
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
              }
              // Early termination
              if (intmask == 0) {
                goto prologue_next_ray5;
              }
              // Update the current mask
              ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
              ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
              // Current scope = 7, scope for t = 6
              rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
            }
            // Early termination
            if (intmask == 0) {
              goto prologue_next_ray5;
            }
            // Update the current mask
            ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
            ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
          }
          // Early termination
          if (intmask == 0) {
            goto prologue_next_ray5;
          }
          // Update the current mask
          mask = _mm_and_ps(mask, mask);
          intmask = intmask & intmask;
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

          i+=4;
        }
 prologue_next_ray5:;
        // Primary loop body
        int e = rays.rayEnd - 3;
        for(;i<e;i+=4){
          {
            // mask is on for active rays
            __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0xffffffff));
            int intmask = 15;
            // Section 0: BEGIN Declaring variables that were written but not read yet

            // Section 0:   END Declaring variables that were written but not read yet

            // scope for O: 5
            __m128 rtsl_O_x = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[0][i])), _mm_set1_ps(center.x()));
            __m128 rtsl_O_y = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[1][i])), _mm_set1_ps(center.y()));
            __m128 rtsl_O_z = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[2][i])), _mm_set1_ps(center.z()));
            // scope for D: 5
            __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
            __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
            __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
            // Current scope = 5, scope for O = 5
            // Current scope = 5, scope for D = 5
            // scope for B: 5
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 5, scope for O = 5
            // Current scope = 5, scope for O = 5
            // scope for C: 5
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
            // Current scope = 5, scope for B = 5
            // Current scope = 5, scope for B = 5
            // Current scope = 5, scope for C = 5
            // scope for disc: 5
            __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_);
            // Current scope = 5, scope for disc = 5
            __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
            __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
            int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
            if(ifintmask4_section_undefined != 0){
              // Current scope = 6, scope for B = 5
              // scope for t: 6
              __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), _mm_set1_ps(rtsl_A_inv));
              // Current scope = 6, scope for D = 5
              // Current scope = 6, scope for t = 6
              // Current scope = 6, scope for O = 5
              rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
              rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
              rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
              // Current scope = 6, scope for O = 5
              // Current scope = 6, scope for D = 5
              // scope for B: 6
              __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
              // Current scope = 6, scope for O = 5
              // Current scope = 6, scope for O = 5
              // scope for C: 6
              __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant1));
              // Current scope = 6, scope for B = 6
              // Current scope = 6, scope for B = 6
              // Current scope = 6, scope for C = 6
              // Current scope = 6, scope for disc = 5
              rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_)));
              // Current scope = 6, scope for disc = 5
              __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
              //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
              __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
              int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
              if(ifintmask8_section_undefined != 0){
                // Current scope = 7, scope for disc = 5
                // scope for r: 7
                __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
                // Current scope = 7, scope for t = 6
                // Current scope = 7, scope for r = 7
                // Current scope = 7, scope for B = 6
                // scope for t1: 7
                __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)));
                // Current scope = 7, scope for t1 = 7
                __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
                int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
                __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
                int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
                if(ifintmask12_section_undefined != 0){
                  // Current scope = 8, scope for r = 7
                  // Current scope = 8, scope for B = 6
                  // Current scope = 8, scope for t = 6
                  rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)))));
                }
                __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
                int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
                if(elseintmask14_section_undefined != 0){
                  // Current scope = 8, scope for t1 = 7
                  // Current scope = 8, scope for t = 6
                  rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
                }
                // Early termination
                if (intmask == 0) {
                  goto for_loop_end6;
                }
                // Update the current mask
                ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
                ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
                // Current scope = 7, scope for t = 6
                rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
              }
              // Early termination
              if (intmask == 0) {
                goto for_loop_end6;
              }
              // Update the current mask
              ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
              ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
            }
            // Early termination
            if (intmask == 0) {
              goto for_loop_end6;
            }
            // Update the current mask
            mask = _mm_and_ps(mask, mask);
            intmask = intmask & intmask;
            // Section 0: BEGIN Storing variables people will need later.

            // Section 0:   END Storing variables people will need later.

            }
 for_loop_end6:;
        }
        // Epilogue
        if(i != rays.rayEnd){
          __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
          // mask is on for active rays
          __m128 mask = _mm_castsi128_ps(_mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i)));
          int intmask = _mm_movemask_ps(mask);
          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

          // scope for O: 5
          __m128 rtsl_O_x = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[0][i])), _mm_set1_ps(center.x()));
          __m128 rtsl_O_y = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[1][i])), _mm_set1_ps(center.y()));
          __m128 rtsl_O_z = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[2][i])), _mm_set1_ps(center.z()));
          // scope for D: 5
          __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
          __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
          __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
          // Current scope = 5, scope for O = 5
          // Current scope = 5, scope for D = 5
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for O = 5
          // Current scope = 5, scope for O = 5
          // scope for C: 5
          __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for C = 5
          // scope for disc: 5
          __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_);
          // Current scope = 5, scope for disc = 5
          __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
          __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
          int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
          if(ifintmask4_section_undefined != 0){
            // Current scope = 6, scope for B = 5
            // scope for t: 6
            __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), _mm_set1_ps(rtsl_A_inv));
            // Current scope = 6, scope for D = 5
            // Current scope = 6, scope for t = 6
            // Current scope = 6, scope for O = 5
            rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
            rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
            rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
            // Current scope = 6, scope for O = 5
            // Current scope = 6, scope for D = 5
            // scope for B: 6
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 6, scope for O = 5
            // Current scope = 6, scope for O = 5
            // scope for C: 6
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant1));
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for C = 6
            // Current scope = 6, scope for disc = 5
            rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_)));
            // Current scope = 6, scope for disc = 5
            __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
            __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
            int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
            if(ifintmask8_section_undefined != 0){
              // Current scope = 7, scope for disc = 5
              // scope for r: 7
              __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
              // Current scope = 7, scope for t = 6
              // Current scope = 7, scope for r = 7
              // Current scope = 7, scope for B = 6
              // scope for t1: 7
              __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)));
              // Current scope = 7, scope for t1 = 7
              __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
              int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
              __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
              int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
              if(ifintmask12_section_undefined != 0){
                // Current scope = 8, scope for r = 7
                // Current scope = 8, scope for B = 6
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)))));
              }
              __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
              int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
              if(elseintmask14_section_undefined != 0){
                // Current scope = 8, scope for t1 = 7
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
              }
              // Early termination
              if (intmask == 0) {
                goto epilogue_next_ray7;
              }
              // Update the current mask
              ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
              ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
              // Current scope = 7, scope for t = 6
              rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
            }
            // Early termination
            if (intmask == 0) {
              goto epilogue_next_ray7;
            }
            // Update the current mask
            ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
            ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
          }
          // Early termination
          if (intmask == 0) {
            goto epilogue_next_ray7;
          }
          // Update the current mask
          mask = _mm_and_ps(mask, mask);
          intmask = intmask & intmask;
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

        }
 epilogue_next_ray7:;
      } else {
      // Single SSE vector
        int i = rays.rayBegin & ~3;
        __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
        __m128 mask = _mm_castsi128_ps(_mm_and_si128(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)), _mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i))));
        int intmask = _mm_movemask_ps(mask);
        // Section 0: BEGIN Declaring variables that were written but not read yet

        // Section 0:   END Declaring variables that were written but not read yet

        // scope for O: 4
        __m128 rtsl_O_x = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[0][i])), _mm_set1_ps(center.x()));
        __m128 rtsl_O_y = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[1][i])), _mm_set1_ps(center.y()));
        __m128 rtsl_O_z = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[2][i])), _mm_set1_ps(center.z()));
        // scope for D: 4
        __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
        __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
        __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
        // Current scope = 4, scope for O = 4
        // Current scope = 4, scope for D = 4
        // scope for B: 4
        __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
        // Current scope = 4, scope for O = 4
        // Current scope = 4, scope for O = 4
        // scope for C: 4
        __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
        // Current scope = 4, scope for B = 4
        // Current scope = 4, scope for B = 4
        // Current scope = 4, scope for C = 4
        // scope for disc: 4
        __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_);
        // Current scope = 4, scope for disc = 4
        __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
        //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
        __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
        int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
        if(ifintmask4_section_undefined != 0){
          // Current scope = 5, scope for B = 4
          // scope for t: 5
          __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), _mm_set1_ps(rtsl_A_inv));
          // Current scope = 5, scope for D = 4
          // Current scope = 5, scope for t = 5
          // Current scope = 5, scope for O = 4
          rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
          rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
          rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
          // Current scope = 5, scope for O = 4
          // Current scope = 5, scope for D = 4
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for O = 4
          // Current scope = 5, scope for O = 4
          // scope for C: 5
          __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant1));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for C = 5
          // Current scope = 5, scope for disc = 4
          rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), rtsl_C_)));
          // Current scope = 5, scope for disc = 4
          __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
          __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
          int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
          if(ifintmask8_section_undefined != 0){
            // Current scope = 6, scope for disc = 4
            // scope for r: 6
            __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
            // Current scope = 6, scope for t = 5
            // Current scope = 6, scope for r = 6
            // Current scope = 6, scope for B = 5
            // scope for t1: 6
            __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)));
            // Current scope = 6, scope for t1 = 6
            __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
            int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
            __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
            int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
            if(ifintmask12_section_undefined != 0){
              // Current scope = 7, scope for r = 6
              // Current scope = 7, scope for B = 5
              // Current scope = 7, scope for t = 5
              rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), _mm_set1_ps(rtsl_A_inv)))));
            }
            __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
            int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
            if(elseintmask14_section_undefined != 0){
              // Current scope = 7, scope for t1 = 6
              // Current scope = 7, scope for t = 5
              rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
            }
            // Early termination
            if (intmask == 0) {
              goto single_vector_next_ray8;
            }
            // Update the current mask
            ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
            ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
            // Current scope = 6, scope for t = 5
            rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
          }
          // Early termination
          if (intmask == 0) {
            goto single_vector_next_ray8;
          }
          // Update the current mask
          ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
          ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
        }
        // Early termination
        if (intmask == 0) {
          goto single_vector_next_ray8;
        }
        // Update the current mask
        mask = _mm_and_ps(mask, mask);
        intmask = intmask & intmask;
        // Section 0: BEGIN Storing variables people will need later.

        // Section 0:   END Storing variables people will need later.

      }
 single_vector_next_ray8:;
#else
      for(int i = rays.begin(); i < rays.end(); i++){
        {
          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

            Vector rtsl_O = (rays.getOrigin(i) - center);
            Vector rtsl_D = rays.getDirection(i);
            Real rtsl_B = Dot(rtsl_O, rtsl_D);
            Real rtsl_C = (Dot(rtsl_O, rtsl_O) - rtsl_invariant0);
            Real rtsl_disc = ((rtsl_B * rtsl_B) - rtsl_C);
            if((rtsl_disc >= 0.f)){
              Real rtsl_t = ((-(rtsl_B)) * rtsl_A_inv);
              rtsl_O += (rtsl_D * rtsl_t);
              Real rtsl_B = Dot(rtsl_O, rtsl_D);
              Real rtsl_C = (Dot(rtsl_O, rtsl_O) - rtsl_invariant1);
              rtsl_disc = ((rtsl_B * rtsl_B) - rtsl_C);
              if((rtsl_disc >= 0.f)){
                Real rtsl_r = Sqrt(rtsl_disc);
                Real rtsl_t1 = (rtsl_t - ((rtsl_r + rtsl_B) * rtsl_A_inv));
                if((rtsl_t1 <= T_EPSILON)){
                  rtsl_t += ((rtsl_r - rtsl_B) * rtsl_A_inv);
                } else {
                  rtsl_t = rtsl_t1;
                }
                rays.hit(i, rtsl_t, getMaterial(), this, getTexCoordMapper());
              }
            }
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

          }
        }
#endif
    }
  } else {
    // !RayPacket::NormalizedDirections
    if(rays.getAllFlags() & RayPacket::ConstantOrigin){
      // RayPacket::ConstantOrigin
      // Special case: constant origin

      // Begin loop invariant setup code
      Vector rtsl_O = (rays.getOrigin(0) - center);
      Real rtsl_C = (Dot(rtsl_O, rtsl_O) - (radius * radius));
      Real rtsl_invariant0 = (radius * radius);
      // End loop invariant setup

      int debugFlag = rays.getAllFlags()&RayPacket::DebugPacket;
      if (debugFlag) {
        cerr << "Sphere::intersect called" << endl;
      }

#ifdef MANTA_SSE
      RayPacketData* data = rays.data;
      if((rays.rayBegin ^ (rays.rayEnd-1)) & ~3){
        int i = rays.rayBegin & ~3;
        // Prologue
        if(i != rays.rayBegin){
          __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
          // mask is on for active rays
          __m128 mask = _mm_castsi128_ps(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)));
          int intmask = _mm_movemask_ps(mask);
          // Section 0: BEGIN Loading variables from previous sections

          __m128 rtsl_O_x = _mm_set1_ps(rtsl_O.x());
          __m128 rtsl_O_y = _mm_set1_ps(rtsl_O.y());
          __m128 rtsl_O_z = _mm_set1_ps(rtsl_O.z());
          // Section 0:   END Loading variables from previous sections

          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

          // scope for D: 5
          __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
          __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
          __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
          // Current scope = 5, scope for D = 5
          // Current scope = 5, scope for D = 5
          // scope for A: 5
          __m128 rtsl_A_ = _mm_add_ps(_mm_mul_ps(rtsl_D_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_D_y, rtsl_D_y), _mm_mul_ps(rtsl_D_z, rtsl_D_z)));
          // Current scope = 5, scope for A = 5
          // scope for A_inv: 5
          __m128 rtsl_A_inv_ = _mm_div_ps(_mm_set1_ps(1), rtsl_A_);
          // Current scope = 5, scope for O = -1
          // Current scope = 5, scope for D = 5
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for A = 5
          // scope for disc: 5
          __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, _mm_set1_ps(rtsl_C)));
          // Current scope = 5, scope for disc = 5
          __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
          __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
          int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
          if(ifintmask4_section_undefined != 0){
            // Current scope = 6, scope for B = 5
            // Current scope = 6, scope for A_inv = 5
            // scope for t: 6
            __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), rtsl_A_inv_);
            // Current scope = 6, scope for D = 5
            // Current scope = 6, scope for t = 6
            // Current scope = 6, scope for O = -1
            rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
            rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
            rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
            // Current scope = 6, scope for O = -1
            // Current scope = 6, scope for D = 5
            // scope for B: 6
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 6, scope for O = -1
            // Current scope = 6, scope for O = -1
            // scope for C: 6
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for A = 5
            // Current scope = 6, scope for C = 6
            // Current scope = 6, scope for disc = 5
            rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_))));
            // Current scope = 6, scope for disc = 5
            __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
            __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
            int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
            if(ifintmask8_section_undefined != 0){
              // Current scope = 7, scope for disc = 5
              // scope for r: 7
              __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
              // Current scope = 7, scope for t = 6
              // Current scope = 7, scope for r = 7
              // Current scope = 7, scope for B = 6
              // Current scope = 7, scope for A_inv = 5
              // scope for t1: 7
              __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_));
              // Current scope = 7, scope for t1 = 7
              __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
              int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
              __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
              int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
              if(ifintmask12_section_undefined != 0){
                // Current scope = 8, scope for r = 7
                // Current scope = 8, scope for B = 6
                // Current scope = 8, scope for A_inv = 5
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_))));
              }
              __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
              int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
              if(elseintmask14_section_undefined != 0){
                // Current scope = 8, scope for t1 = 7
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
              }
              // Early termination
              if (intmask == 0) {
                goto prologue_next_ray10;
              }
              // Update the current mask
              ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
              ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
              // Current scope = 7, scope for t = 6
              rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
            }
            // Early termination
            if (intmask == 0) {
              goto prologue_next_ray10;
            }
            // Update the current mask
            ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
            ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
          }
          // Early termination
          if (intmask == 0) {
            goto prologue_next_ray10;
          }
          // Update the current mask
          mask = _mm_and_ps(mask, mask);
          intmask = intmask & intmask;
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

          i+=4;
        }
 prologue_next_ray10:;
        // Primary loop body
        int e = rays.rayEnd - 3;
        for(;i<e;i+=4){
          {
            // mask is on for active rays
            __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0xffffffff));
            int intmask = 15;
            // Section 0: BEGIN Loading variables from previous sections

            __m128 rtsl_O_x = _mm_set1_ps(rtsl_O.x());
            __m128 rtsl_O_y = _mm_set1_ps(rtsl_O.y());
            __m128 rtsl_O_z = _mm_set1_ps(rtsl_O.z());
            // Section 0:   END Loading variables from previous sections

            // Section 0: BEGIN Declaring variables that were written but not read yet

            // Section 0:   END Declaring variables that were written but not read yet

            // scope for D: 5
            __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
            __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
            __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
            // Current scope = 5, scope for D = 5
            // Current scope = 5, scope for D = 5
            // scope for A: 5
            __m128 rtsl_A_ = _mm_add_ps(_mm_mul_ps(rtsl_D_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_D_y, rtsl_D_y), _mm_mul_ps(rtsl_D_z, rtsl_D_z)));
            // Current scope = 5, scope for A = 5
            // scope for A_inv: 5
            __m128 rtsl_A_inv_ = _mm_div_ps(_mm_set1_ps(1), rtsl_A_);
            // Current scope = 5, scope for O = -1
            // Current scope = 5, scope for D = 5
            // scope for B: 5
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 5, scope for B = 5
            // Current scope = 5, scope for B = 5
            // Current scope = 5, scope for A = 5
            // scope for disc: 5
            __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, _mm_set1_ps(rtsl_C)));
            // Current scope = 5, scope for disc = 5
            __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
            __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
            int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
            if(ifintmask4_section_undefined != 0){
              // Current scope = 6, scope for B = 5
              // Current scope = 6, scope for A_inv = 5
              // scope for t: 6
              __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), rtsl_A_inv_);
              // Current scope = 6, scope for D = 5
              // Current scope = 6, scope for t = 6
              // Current scope = 6, scope for O = -1
              rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
              rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
              rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
              // Current scope = 6, scope for O = -1
              // Current scope = 6, scope for D = 5
              // scope for B: 6
              __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
              // Current scope = 6, scope for O = -1
              // Current scope = 6, scope for O = -1
              // scope for C: 6
              __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
              // Current scope = 6, scope for B = 6
              // Current scope = 6, scope for B = 6
              // Current scope = 6, scope for A = 5
              // Current scope = 6, scope for C = 6
              // Current scope = 6, scope for disc = 5
              rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_))));
              // Current scope = 6, scope for disc = 5
              __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
              //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
              __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
              int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
              if(ifintmask8_section_undefined != 0){
                // Current scope = 7, scope for disc = 5
                // scope for r: 7
                __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
                // Current scope = 7, scope for t = 6
                // Current scope = 7, scope for r = 7
                // Current scope = 7, scope for B = 6
                // Current scope = 7, scope for A_inv = 5
                // scope for t1: 7
                __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_));
                // Current scope = 7, scope for t1 = 7
                __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
                int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
                __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
                int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
                if(ifintmask12_section_undefined != 0){
                  // Current scope = 8, scope for r = 7
                  // Current scope = 8, scope for B = 6
                  // Current scope = 8, scope for A_inv = 5
                  // Current scope = 8, scope for t = 6
                  rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_))));
                }
                __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
                int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
                if(elseintmask14_section_undefined != 0){
                  // Current scope = 8, scope for t1 = 7
                  // Current scope = 8, scope for t = 6
                  rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
                }
                // Early termination
                if (intmask == 0) {
                  goto for_loop_end11;
                }
                // Update the current mask
                ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
                ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
                // Current scope = 7, scope for t = 6
                rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
              }
              // Early termination
              if (intmask == 0) {
                goto for_loop_end11;
              }
              // Update the current mask
              ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
              ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
            }
            // Early termination
            if (intmask == 0) {
              goto for_loop_end11;
            }
            // Update the current mask
            mask = _mm_and_ps(mask, mask);
            intmask = intmask & intmask;
            // Section 0: BEGIN Storing variables people will need later.

            // Section 0:   END Storing variables people will need later.

            }
 for_loop_end11:;
        }
        // Epilogue
        if(i != rays.rayEnd){
          __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
          // mask is on for active rays
          __m128 mask = _mm_castsi128_ps(_mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i)));
          int intmask = _mm_movemask_ps(mask);
          // Section 0: BEGIN Loading variables from previous sections

          __m128 rtsl_O_x = _mm_set1_ps(rtsl_O.x());
          __m128 rtsl_O_y = _mm_set1_ps(rtsl_O.y());
          __m128 rtsl_O_z = _mm_set1_ps(rtsl_O.z());
          // Section 0:   END Loading variables from previous sections

          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

          // scope for D: 5
          __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
          __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
          __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
          // Current scope = 5, scope for D = 5
          // Current scope = 5, scope for D = 5
          // scope for A: 5
          __m128 rtsl_A_ = _mm_add_ps(_mm_mul_ps(rtsl_D_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_D_y, rtsl_D_y), _mm_mul_ps(rtsl_D_z, rtsl_D_z)));
          // Current scope = 5, scope for A = 5
          // scope for A_inv: 5
          __m128 rtsl_A_inv_ = _mm_div_ps(_mm_set1_ps(1), rtsl_A_);
          // Current scope = 5, scope for O = -1
          // Current scope = 5, scope for D = 5
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for A = 5
          // scope for disc: 5
          __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, _mm_set1_ps(rtsl_C)));
          // Current scope = 5, scope for disc = 5
          __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
          __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
          int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
          if(ifintmask4_section_undefined != 0){
            // Current scope = 6, scope for B = 5
            // Current scope = 6, scope for A_inv = 5
            // scope for t: 6
            __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), rtsl_A_inv_);
            // Current scope = 6, scope for D = 5
            // Current scope = 6, scope for t = 6
            // Current scope = 6, scope for O = -1
            rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
            rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
            rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
            // Current scope = 6, scope for O = -1
            // Current scope = 6, scope for D = 5
            // scope for B: 6
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 6, scope for O = -1
            // Current scope = 6, scope for O = -1
            // scope for C: 6
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for A = 5
            // Current scope = 6, scope for C = 6
            // Current scope = 6, scope for disc = 5
            rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_))));
            // Current scope = 6, scope for disc = 5
            __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
            __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
            int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
            if(ifintmask8_section_undefined != 0){
              // Current scope = 7, scope for disc = 5
              // scope for r: 7
              __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
              // Current scope = 7, scope for t = 6
              // Current scope = 7, scope for r = 7
              // Current scope = 7, scope for B = 6
              // Current scope = 7, scope for A_inv = 5
              // scope for t1: 7
              __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_));
              // Current scope = 7, scope for t1 = 7
              __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
              int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
              __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
              int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
              if(ifintmask12_section_undefined != 0){
                // Current scope = 8, scope for r = 7
                // Current scope = 8, scope for B = 6
                // Current scope = 8, scope for A_inv = 5
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_))));
              }
              __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
              int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
              if(elseintmask14_section_undefined != 0){
                // Current scope = 8, scope for t1 = 7
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
              }
              // Early termination
              if (intmask == 0) {
                goto epilogue_next_ray12;
              }
              // Update the current mask
              ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
              ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
              // Current scope = 7, scope for t = 6
              rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
            }
            // Early termination
            if (intmask == 0) {
              goto epilogue_next_ray12;
            }
            // Update the current mask
            ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
            ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
          }
          // Early termination
          if (intmask == 0) {
            goto epilogue_next_ray12;
          }
          // Update the current mask
          mask = _mm_and_ps(mask, mask);
          intmask = intmask & intmask;
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

        }
 epilogue_next_ray12:;
      } else {
      // Single SSE vector
        int i = rays.rayBegin & ~3;
        __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
        __m128 mask = _mm_castsi128_ps(_mm_and_si128(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)), _mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i))));
        int intmask = _mm_movemask_ps(mask);
        // Section 0: BEGIN Loading variables from previous sections

        __m128 rtsl_O_x = _mm_set1_ps(rtsl_O.x());
        __m128 rtsl_O_y = _mm_set1_ps(rtsl_O.y());
        __m128 rtsl_O_z = _mm_set1_ps(rtsl_O.z());
        // Section 0:   END Loading variables from previous sections

        // Section 0: BEGIN Declaring variables that were written but not read yet

        // Section 0:   END Declaring variables that were written but not read yet

        // scope for D: 4
        __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
        __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
        __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
        // Current scope = 4, scope for D = 4
        // Current scope = 4, scope for D = 4
        // scope for A: 4
        __m128 rtsl_A_ = _mm_add_ps(_mm_mul_ps(rtsl_D_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_D_y, rtsl_D_y), _mm_mul_ps(rtsl_D_z, rtsl_D_z)));
        // Current scope = 4, scope for A = 4
        // scope for A_inv: 4
        __m128 rtsl_A_inv_ = _mm_div_ps(_mm_set1_ps(1), rtsl_A_);
        // Current scope = 4, scope for O = -1
        // Current scope = 4, scope for D = 4
        // scope for B: 4
        __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
        // Current scope = 4, scope for B = 4
        // Current scope = 4, scope for B = 4
        // Current scope = 4, scope for A = 4
        // scope for disc: 4
        __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, _mm_set1_ps(rtsl_C)));
        // Current scope = 4, scope for disc = 4
        __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
        //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
        __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
        int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
        if(ifintmask4_section_undefined != 0){
          // Current scope = 5, scope for B = 4
          // Current scope = 5, scope for A_inv = 4
          // scope for t: 5
          __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), rtsl_A_inv_);
          // Current scope = 5, scope for D = 4
          // Current scope = 5, scope for t = 5
          // Current scope = 5, scope for O = -1
          rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
          rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
          rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
          // Current scope = 5, scope for O = -1
          // Current scope = 5, scope for D = 4
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for O = -1
          // Current scope = 5, scope for O = -1
          // scope for C: 5
          __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for A = 4
          // Current scope = 5, scope for C = 5
          // Current scope = 5, scope for disc = 4
          rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_))));
          // Current scope = 5, scope for disc = 4
          __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
          __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
          int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
          if(ifintmask8_section_undefined != 0){
            // Current scope = 6, scope for disc = 4
            // scope for r: 6
            __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
            // Current scope = 6, scope for t = 5
            // Current scope = 6, scope for r = 6
            // Current scope = 6, scope for B = 5
            // Current scope = 6, scope for A_inv = 4
            // scope for t1: 6
            __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_));
            // Current scope = 6, scope for t1 = 6
            __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
            int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
            __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
            int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
            if(ifintmask12_section_undefined != 0){
              // Current scope = 7, scope for r = 6
              // Current scope = 7, scope for B = 5
              // Current scope = 7, scope for A_inv = 4
              // Current scope = 7, scope for t = 5
              rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_))));
            }
            __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
            int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
            if(elseintmask14_section_undefined != 0){
              // Current scope = 7, scope for t1 = 6
              // Current scope = 7, scope for t = 5
              rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
            }
            // Early termination
            if (intmask == 0) {
              goto single_vector_next_ray13;
            }
            // Update the current mask
            ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
            ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
            // Current scope = 6, scope for t = 5
            rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
          }
          // Early termination
          if (intmask == 0) {
            goto single_vector_next_ray13;
          }
          // Update the current mask
          ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
          ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
        }
        // Early termination
        if (intmask == 0) {
          goto single_vector_next_ray13;
        }
        // Update the current mask
        mask = _mm_and_ps(mask, mask);
        intmask = intmask & intmask;
        // Section 0: BEGIN Storing variables people will need later.

        // Section 0:   END Storing variables people will need later.

      }
 single_vector_next_ray13:;
#else
      for(int i = rays.begin(); i < rays.end(); i++){
        {
          // Section 0: BEGIN Loading variables from previous sections

          Vector rtsl_O(rays.getOrigin(i)-center);
          // Section 0:   END Loading variables from previous sections

          // Section 0: BEGIN Declaring variables that were written but not read yet

           // No declaration for O
          // Section 0:   END Declaring variables that were written but not read yet

            Vector rtsl_D = rays.getDirection(i);
            Real rtsl_A = Dot(rtsl_D, rtsl_D);
            Real rtsl_A_inv = (1 / rtsl_A);
            Real rtsl_B = Dot(rtsl_O, rtsl_D);
            Real rtsl_disc = ((rtsl_B * rtsl_B) - (rtsl_A * rtsl_C));
            if((rtsl_disc >= 0.f)){
              Real rtsl_t = ((-(rtsl_B)) * rtsl_A_inv);
              rtsl_O += (rtsl_D * rtsl_t);
              Real rtsl_B = Dot(rtsl_O, rtsl_D);
              Real rtsl_C = (Dot(rtsl_O, rtsl_O) - rtsl_invariant0);
              rtsl_disc = ((rtsl_B * rtsl_B) - (rtsl_A * rtsl_C));
              if((rtsl_disc >= 0.f)){
                Real rtsl_r = Sqrt(rtsl_disc);
                Real rtsl_t1 = (rtsl_t - ((rtsl_r + rtsl_B) * rtsl_A_inv));
                if((rtsl_t1 <= T_EPSILON)){
                  rtsl_t += ((rtsl_r - rtsl_B) * rtsl_A_inv);
                } else {
                  rtsl_t = rtsl_t1;
                }
                rays.hit(i, rtsl_t, getMaterial(), this, getTexCoordMapper());
              }
            }
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

          }
        }
#endif
    } else {
      // !RayPacket::ConstantOrigin
      // General case:

      // Begin loop invariant setup code
      Real rtsl_invariant0 = (radius * radius);
      Real rtsl_invariant1 = (radius * radius);
      // End loop invariant setup

      int debugFlag = rays.getAllFlags()&RayPacket::DebugPacket;
      if (debugFlag) {
        cerr << "Sphere::intersect called" << endl;
      }

#ifdef MANTA_SSE
      RayPacketData* data = rays.data;
      if((rays.rayBegin ^ (rays.rayEnd-1)) & ~3){
        int i = rays.rayBegin & ~3;
        // Prologue
        if(i != rays.rayBegin){
          __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
          // mask is on for active rays
          __m128 mask = _mm_castsi128_ps(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)));
          int intmask = _mm_movemask_ps(mask);
          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

          // scope for O: 5
          __m128 rtsl_O_x = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[0][i])), _mm_set1_ps(center.x()));
          __m128 rtsl_O_y = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[1][i])), _mm_set1_ps(center.y()));
          __m128 rtsl_O_z = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[2][i])), _mm_set1_ps(center.z()));
          // scope for D: 5
          __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
          __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
          __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
          // Current scope = 5, scope for D = 5
          // Current scope = 5, scope for D = 5
          // scope for A: 5
          __m128 rtsl_A_ = _mm_add_ps(_mm_mul_ps(rtsl_D_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_D_y, rtsl_D_y), _mm_mul_ps(rtsl_D_z, rtsl_D_z)));
          // Current scope = 5, scope for A = 5
          // scope for A_inv: 5
          __m128 rtsl_A_inv_ = _mm_div_ps(_mm_set1_ps(1), rtsl_A_);
          // Current scope = 5, scope for O = 5
          // Current scope = 5, scope for D = 5
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for O = 5
          // Current scope = 5, scope for O = 5
          // scope for C: 5
          __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for A = 5
          // Current scope = 5, scope for C = 5
          // scope for disc: 5
          __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_));
          // Current scope = 5, scope for disc = 5
          __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
          __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
          int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
          if(ifintmask4_section_undefined != 0){
            // Current scope = 6, scope for B = 5
            // Current scope = 6, scope for A_inv = 5
            // scope for t: 6
            __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), rtsl_A_inv_);
            // Current scope = 6, scope for D = 5
            // Current scope = 6, scope for t = 6
            // Current scope = 6, scope for O = 5
            rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
            rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
            rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
            // Current scope = 6, scope for O = 5
            // Current scope = 6, scope for D = 5
            // scope for B: 6
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 6, scope for O = 5
            // Current scope = 6, scope for O = 5
            // scope for C: 6
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant1));
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for A = 5
            // Current scope = 6, scope for C = 6
            // Current scope = 6, scope for disc = 5
            rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_))));
            // Current scope = 6, scope for disc = 5
            __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
            __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
            int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
            if(ifintmask8_section_undefined != 0){
              // Current scope = 7, scope for disc = 5
              // scope for r: 7
              __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
              // Current scope = 7, scope for t = 6
              // Current scope = 7, scope for r = 7
              // Current scope = 7, scope for B = 6
              // Current scope = 7, scope for A_inv = 5
              // scope for t1: 7
              __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_));
              // Current scope = 7, scope for t1 = 7
              __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
              int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
              __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
              int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
              if(ifintmask12_section_undefined != 0){
                // Current scope = 8, scope for r = 7
                // Current scope = 8, scope for B = 6
                // Current scope = 8, scope for A_inv = 5
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_))));
              }
              __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
              int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
              if(elseintmask14_section_undefined != 0){
                // Current scope = 8, scope for t1 = 7
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
              }
              // Early termination
              if (intmask == 0) {
                goto prologue_next_ray15;
              }
              // Update the current mask
              ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
              ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
              // Current scope = 7, scope for t = 6
              rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
            }
            // Early termination
            if (intmask == 0) {
              goto prologue_next_ray15;
            }
            // Update the current mask
            ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
            ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
          }
          // Early termination
          if (intmask == 0) {
            goto prologue_next_ray15;
          }
          // Update the current mask
          mask = _mm_and_ps(mask, mask);
          intmask = intmask & intmask;
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

          i+=4;
        }
 prologue_next_ray15:;
        // Primary loop body
        int e = rays.rayEnd - 3;
        for(;i<e;i+=4){
          {
            // mask is on for active rays
            __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0xffffffff));
            int intmask = 15;
            // Section 0: BEGIN Declaring variables that were written but not read yet

            // Section 0:   END Declaring variables that were written but not read yet

            // scope for O: 5
            __m128 rtsl_O_x = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[0][i])), _mm_set1_ps(center.x()));
            __m128 rtsl_O_y = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[1][i])), _mm_set1_ps(center.y()));
            __m128 rtsl_O_z = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[2][i])), _mm_set1_ps(center.z()));
            // scope for D: 5
            __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
            __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
            __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
            // Current scope = 5, scope for D = 5
            // Current scope = 5, scope for D = 5
            // scope for A: 5
            __m128 rtsl_A_ = _mm_add_ps(_mm_mul_ps(rtsl_D_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_D_y, rtsl_D_y), _mm_mul_ps(rtsl_D_z, rtsl_D_z)));
            // Current scope = 5, scope for A = 5
            // scope for A_inv: 5
            __m128 rtsl_A_inv_ = _mm_div_ps(_mm_set1_ps(1), rtsl_A_);
            // Current scope = 5, scope for O = 5
            // Current scope = 5, scope for D = 5
            // scope for B: 5
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 5, scope for O = 5
            // Current scope = 5, scope for O = 5
            // scope for C: 5
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
            // Current scope = 5, scope for B = 5
            // Current scope = 5, scope for B = 5
            // Current scope = 5, scope for A = 5
            // Current scope = 5, scope for C = 5
            // scope for disc: 5
            __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_));
            // Current scope = 5, scope for disc = 5
            __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
            __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
            int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
            if(ifintmask4_section_undefined != 0){
              // Current scope = 6, scope for B = 5
              // Current scope = 6, scope for A_inv = 5
              // scope for t: 6
              __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), rtsl_A_inv_);
              // Current scope = 6, scope for D = 5
              // Current scope = 6, scope for t = 6
              // Current scope = 6, scope for O = 5
              rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
              rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
              rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
              // Current scope = 6, scope for O = 5
              // Current scope = 6, scope for D = 5
              // scope for B: 6
              __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
              // Current scope = 6, scope for O = 5
              // Current scope = 6, scope for O = 5
              // scope for C: 6
              __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant1));
              // Current scope = 6, scope for B = 6
              // Current scope = 6, scope for B = 6
              // Current scope = 6, scope for A = 5
              // Current scope = 6, scope for C = 6
              // Current scope = 6, scope for disc = 5
              rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_))));
              // Current scope = 6, scope for disc = 5
              __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
              //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
              __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
              int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
              if(ifintmask8_section_undefined != 0){
                // Current scope = 7, scope for disc = 5
                // scope for r: 7
                __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
                // Current scope = 7, scope for t = 6
                // Current scope = 7, scope for r = 7
                // Current scope = 7, scope for B = 6
                // Current scope = 7, scope for A_inv = 5
                // scope for t1: 7
                __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_));
                // Current scope = 7, scope for t1 = 7
                __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
                int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
                __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
                int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
                if(ifintmask12_section_undefined != 0){
                  // Current scope = 8, scope for r = 7
                  // Current scope = 8, scope for B = 6
                  // Current scope = 8, scope for A_inv = 5
                  // Current scope = 8, scope for t = 6
                  rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_))));
                }
                __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
                int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
                if(elseintmask14_section_undefined != 0){
                  // Current scope = 8, scope for t1 = 7
                  // Current scope = 8, scope for t = 6
                  rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
                }
                // Early termination
                if (intmask == 0) {
                  goto for_loop_end16;
                }
                // Update the current mask
                ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
                ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
                // Current scope = 7, scope for t = 6
                rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
              }
              // Early termination
              if (intmask == 0) {
                goto for_loop_end16;
              }
              // Update the current mask
              ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
              ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
            }
            // Early termination
            if (intmask == 0) {
              goto for_loop_end16;
            }
            // Update the current mask
            mask = _mm_and_ps(mask, mask);
            intmask = intmask & intmask;
            // Section 0: BEGIN Storing variables people will need later.

            // Section 0:   END Storing variables people will need later.

            }
 for_loop_end16:;
        }
        // Epilogue
        if(i != rays.rayEnd){
          __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
          // mask is on for active rays
          __m128 mask = _mm_castsi128_ps(_mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i)));
          int intmask = _mm_movemask_ps(mask);
          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

          // scope for O: 5
          __m128 rtsl_O_x = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[0][i])), _mm_set1_ps(center.x()));
          __m128 rtsl_O_y = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[1][i])), _mm_set1_ps(center.y()));
          __m128 rtsl_O_z = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[2][i])), _mm_set1_ps(center.z()));
          // scope for D: 5
          __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
          __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
          __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
          // Current scope = 5, scope for D = 5
          // Current scope = 5, scope for D = 5
          // scope for A: 5
          __m128 rtsl_A_ = _mm_add_ps(_mm_mul_ps(rtsl_D_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_D_y, rtsl_D_y), _mm_mul_ps(rtsl_D_z, rtsl_D_z)));
          // Current scope = 5, scope for A = 5
          // scope for A_inv: 5
          __m128 rtsl_A_inv_ = _mm_div_ps(_mm_set1_ps(1), rtsl_A_);
          // Current scope = 5, scope for O = 5
          // Current scope = 5, scope for D = 5
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for O = 5
          // Current scope = 5, scope for O = 5
          // scope for C: 5
          __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for A = 5
          // Current scope = 5, scope for C = 5
          // scope for disc: 5
          __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_));
          // Current scope = 5, scope for disc = 5
          __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
          __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
          int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
          if(ifintmask4_section_undefined != 0){
            // Current scope = 6, scope for B = 5
            // Current scope = 6, scope for A_inv = 5
            // scope for t: 6
            __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), rtsl_A_inv_);
            // Current scope = 6, scope for D = 5
            // Current scope = 6, scope for t = 6
            // Current scope = 6, scope for O = 5
            rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
            rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
            rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
            // Current scope = 6, scope for O = 5
            // Current scope = 6, scope for D = 5
            // scope for B: 6
            __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
            // Current scope = 6, scope for O = 5
            // Current scope = 6, scope for O = 5
            // scope for C: 6
            __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant1));
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for B = 6
            // Current scope = 6, scope for A = 5
            // Current scope = 6, scope for C = 6
            // Current scope = 6, scope for disc = 5
            rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_))));
            // Current scope = 6, scope for disc = 5
            __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
            //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
            __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
            int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
            if(ifintmask8_section_undefined != 0){
              // Current scope = 7, scope for disc = 5
              // scope for r: 7
              __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
              // Current scope = 7, scope for t = 6
              // Current scope = 7, scope for r = 7
              // Current scope = 7, scope for B = 6
              // Current scope = 7, scope for A_inv = 5
              // scope for t1: 7
              __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_));
              // Current scope = 7, scope for t1 = 7
              __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
              int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
              __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
              int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
              if(ifintmask12_section_undefined != 0){
                // Current scope = 8, scope for r = 7
                // Current scope = 8, scope for B = 6
                // Current scope = 8, scope for A_inv = 5
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_))));
              }
              __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
              int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
              if(elseintmask14_section_undefined != 0){
                // Current scope = 8, scope for t1 = 7
                // Current scope = 8, scope for t = 6
                rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
              }
              // Early termination
              if (intmask == 0) {
                goto epilogue_next_ray17;
              }
              // Update the current mask
              ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
              ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
              // Current scope = 7, scope for t = 6
              rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
            }
            // Early termination
            if (intmask == 0) {
              goto epilogue_next_ray17;
            }
            // Update the current mask
            ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
            ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
          }
          // Early termination
          if (intmask == 0) {
            goto epilogue_next_ray17;
          }
          // Update the current mask
          mask = _mm_and_ps(mask, mask);
          intmask = intmask & intmask;
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

        }
 epilogue_next_ray17:;
      } else {
      // Single SSE vector
        int i = rays.rayBegin & ~3;
        __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
        __m128 mask = _mm_castsi128_ps(_mm_and_si128(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)), _mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i))));
        int intmask = _mm_movemask_ps(mask);
        // Section 0: BEGIN Declaring variables that were written but not read yet

        // Section 0:   END Declaring variables that were written but not read yet

        // scope for O: 4
        __m128 rtsl_O_x = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[0][i])), _mm_set1_ps(center.x()));
        __m128 rtsl_O_y = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[1][i])), _mm_set1_ps(center.y()));
        __m128 rtsl_O_z = _mm_sub_ps(_mm_load_ps((float*)&(data->origin[2][i])), _mm_set1_ps(center.z()));
        // scope for D: 4
        __m128 rtsl_D_x = _mm_load_ps((float*)&(data->direction[0][i]));
        __m128 rtsl_D_y = _mm_load_ps((float*)&(data->direction[1][i]));
        __m128 rtsl_D_z = _mm_load_ps((float*)&(data->direction[2][i]));
        // Current scope = 4, scope for D = 4
        // Current scope = 4, scope for D = 4
        // scope for A: 4
        __m128 rtsl_A_ = _mm_add_ps(_mm_mul_ps(rtsl_D_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_D_y, rtsl_D_y), _mm_mul_ps(rtsl_D_z, rtsl_D_z)));
        // Current scope = 4, scope for A = 4
        // scope for A_inv: 4
        __m128 rtsl_A_inv_ = _mm_div_ps(_mm_set1_ps(1), rtsl_A_);
        // Current scope = 4, scope for O = 4
        // Current scope = 4, scope for D = 4
        // scope for B: 4
        __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
        // Current scope = 4, scope for O = 4
        // Current scope = 4, scope for O = 4
        // scope for C: 4
        __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant0));
        // Current scope = 4, scope for B = 4
        // Current scope = 4, scope for B = 4
        // Current scope = 4, scope for A = 4
        // Current scope = 4, scope for C = 4
        // scope for disc: 4
        __m128 rtsl_disc_ = _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_));
        // Current scope = 4, scope for disc = 4
        __m128 condmask1_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
        //int condintmask2_section_undefined = _mm_movemask_ps(condmask1_section_undefined);
        __m128 ifmask3_section_undefined = _mm_and_ps(mask, condmask1_section_undefined);
        int ifintmask4_section_undefined = _mm_movemask_ps(ifmask3_section_undefined);
        if(ifintmask4_section_undefined != 0){
          // Current scope = 5, scope for B = 4
          // Current scope = 5, scope for A_inv = 4
          // scope for t: 5
          __m128 rtsl_t_ = _mm_mul_ps(_mm_xor_ps(rtsl_B_, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))), rtsl_A_inv_);
          // Current scope = 5, scope for D = 4
          // Current scope = 5, scope for t = 5
          // Current scope = 5, scope for O = 4
          rtsl_O_x = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_x), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_x, _mm_mul_ps(rtsl_D_x, rtsl_t_))));
          rtsl_O_y = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_y), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_y, _mm_mul_ps(rtsl_D_y, rtsl_t_))));
          rtsl_O_z = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_O_z), _mm_and_ps(ifmask3_section_undefined, _mm_add_ps(rtsl_O_z, _mm_mul_ps(rtsl_D_z, rtsl_t_))));
          // Current scope = 5, scope for O = 4
          // Current scope = 5, scope for D = 4
          // scope for B: 5
          __m128 rtsl_B_ = _mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_D_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_D_y), _mm_mul_ps(rtsl_O_z, rtsl_D_z)));
          // Current scope = 5, scope for O = 4
          // Current scope = 5, scope for O = 4
          // scope for C: 5
          __m128 rtsl_C_ = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(rtsl_O_x, rtsl_O_x), _mm_add_ps(_mm_mul_ps(rtsl_O_y, rtsl_O_y), _mm_mul_ps(rtsl_O_z, rtsl_O_z))), _mm_set1_ps(rtsl_invariant1));
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for B = 5
          // Current scope = 5, scope for A = 4
          // Current scope = 5, scope for C = 5
          // Current scope = 5, scope for disc = 4
          rtsl_disc_ = _mm_or_ps(_mm_andnot_ps(ifmask3_section_undefined, rtsl_disc_), _mm_and_ps(ifmask3_section_undefined, _mm_sub_ps(_mm_mul_ps(rtsl_B_, rtsl_B_), _mm_mul_ps(rtsl_A_, rtsl_C_))));
          // Current scope = 5, scope for disc = 4
          __m128 condmask5_section_undefined = _mm_cmpge_ps(rtsl_disc_, _mm_set1_ps(0));
          //int condintmask6_section_undefined = _mm_movemask_ps(condmask5_section_undefined);
          __m128 ifmask7_section_undefined = _mm_and_ps(ifmask3_section_undefined, condmask5_section_undefined);
          int ifintmask8_section_undefined = _mm_movemask_ps(ifmask7_section_undefined);
          if(ifintmask8_section_undefined != 0){
            // Current scope = 6, scope for disc = 4
            // scope for r: 6
            __m128 rtsl_r_ = _mm_sqrt_ps(rtsl_disc_);
            // Current scope = 6, scope for t = 5
            // Current scope = 6, scope for r = 6
            // Current scope = 6, scope for B = 5
            // Current scope = 6, scope for A_inv = 4
            // scope for t1: 6
            __m128 rtsl_t1_ = _mm_sub_ps(rtsl_t_, _mm_mul_ps(_mm_add_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_));
            // Current scope = 6, scope for t1 = 6
            __m128 condmask9_section_undefined = _mm_cmple_ps(rtsl_t1_, _mm_set1_ps(T_EPSILON));
            int condintmask10_section_undefined = _mm_movemask_ps(condmask9_section_undefined);
            __m128 ifmask11_section_undefined = _mm_and_ps(ifmask7_section_undefined, condmask9_section_undefined);
            int ifintmask12_section_undefined = _mm_movemask_ps(ifmask11_section_undefined);
            if(ifintmask12_section_undefined != 0){
              // Current scope = 7, scope for r = 6
              // Current scope = 7, scope for B = 5
              // Current scope = 7, scope for A_inv = 4
              // Current scope = 7, scope for t = 5
              rtsl_t_ = _mm_or_ps(_mm_andnot_ps(ifmask11_section_undefined, rtsl_t_), _mm_and_ps(ifmask11_section_undefined, _mm_add_ps(rtsl_t_, _mm_mul_ps(_mm_sub_ps(rtsl_r_, rtsl_B_), rtsl_A_inv_))));
            }
            __m128 elsemask13_section_undefined = _mm_andnot_ps(condmask9_section_undefined, ifmask7_section_undefined);
            int elseintmask14_section_undefined = ~condintmask10_section_undefined&ifintmask8_section_undefined;
            if(elseintmask14_section_undefined != 0){
              // Current scope = 7, scope for t1 = 6
              // Current scope = 7, scope for t = 5
              rtsl_t_ = _mm_or_ps(_mm_andnot_ps(elsemask13_section_undefined, rtsl_t_), _mm_and_ps(elsemask13_section_undefined, rtsl_t1_));
            }
            // Early termination
            if (intmask == 0) {
              goto single_vector_next_ray18;
            }
            // Update the current mask
            ifmask7_section_undefined = _mm_and_ps(mask, ifmask7_section_undefined);
            ifintmask8_section_undefined = intmask & ifintmask8_section_undefined;
            // Current scope = 6, scope for t = 5
            rays.hitWithMask(i, ifmask7_section_undefined, rtsl_t_, getMaterial(), this, getTexCoordMapper());
          }
          // Early termination
          if (intmask == 0) {
            goto single_vector_next_ray18;
          }
          // Update the current mask
          ifmask3_section_undefined = _mm_and_ps(mask, ifmask3_section_undefined);
          ifintmask4_section_undefined = intmask & ifintmask4_section_undefined;
        }
        // Early termination
        if (intmask == 0) {
          goto single_vector_next_ray18;
        }
        // Update the current mask
        mask = _mm_and_ps(mask, mask);
        intmask = intmask & intmask;
        // Section 0: BEGIN Storing variables people will need later.

        // Section 0:   END Storing variables people will need later.

      }
 single_vector_next_ray18:;
#else
      for(int i = rays.begin(); i < rays.end(); i++){
        {
          // Section 0: BEGIN Declaring variables that were written but not read yet

          // Section 0:   END Declaring variables that were written but not read yet

            Vector rtsl_O = (rays.getOrigin(i) - center);
            Vector rtsl_D = rays.getDirection(i);
            Real rtsl_A = Dot(rtsl_D, rtsl_D);
            Real rtsl_A_inv = (1 / rtsl_A);
            Real rtsl_B = Dot(rtsl_O, rtsl_D);
            Real rtsl_C = (Dot(rtsl_O, rtsl_O) - rtsl_invariant0);
            Real rtsl_disc = ((rtsl_B * rtsl_B) - (rtsl_A * rtsl_C));
            if((rtsl_disc >= 0.f)){
              Real rtsl_t = ((-(rtsl_B)) * rtsl_A_inv);
              rtsl_O += (rtsl_D * rtsl_t);
              Real rtsl_B = Dot(rtsl_O, rtsl_D);
              Real rtsl_C = (Dot(rtsl_O, rtsl_O) - rtsl_invariant1);
              rtsl_disc = ((rtsl_B * rtsl_B) - (rtsl_A * rtsl_C));
              if((rtsl_disc >= 0.f)){
                Real rtsl_r = Sqrt(rtsl_disc);
                Real rtsl_t1 = (rtsl_t - ((rtsl_r + rtsl_B) * rtsl_A_inv));
                if((rtsl_t1 <= T_EPSILON)){
                  rtsl_t += ((rtsl_r - rtsl_B) * rtsl_A_inv);
                } else {
                  rtsl_t = rtsl_t1;
                }
                rays.hit(i, rtsl_t, getMaterial(), this, getTexCoordMapper());
              }
            }
          // Section 0: BEGIN Storing variables people will need later.

          // Section 0:   END Storing variables people will need later.

          }
        }
#endif
    }
  }
}

void Sphere::computeNormal(const RenderContext& context, RayPacket& rays) const
{
  rays.computeHitPositions();
  // General case:
  int debugFlag = rays.getAllFlags()&RayPacket::DebugPacket;
  if (debugFlag) {
    cerr << "Sphere::computeNormal called" << endl;
  }

#ifdef MANTA_SSE
  RayPacketData* data = rays.data;
  if((rays.rayBegin ^ (rays.rayEnd-1)) & ~3){
    int i = rays.rayBegin & ~3;
    // Prologue
    if(i != rays.rayBegin){
      __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
      // mask is on for active rays
      __m128 mask = _mm_castsi128_ps(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)));
      //int intmask = _mm_movemask_ps(mask);
      _mm_store_ps((float*)&(data->normal[0][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[0][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[0][i])), _mm_set1_ps(center.x())), _mm_set1_ps(inv_radius)))));
      _mm_store_ps((float*)&(data->normal[1][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[1][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[1][i])), _mm_set1_ps(center.y())), _mm_set1_ps(inv_radius)))));
      _mm_store_ps((float*)&(data->normal[2][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[2][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[2][i])), _mm_set1_ps(center.z())), _mm_set1_ps(inv_radius)))));
      i+=4;
    }
    // Primary loop body
    int e = rays.rayEnd - 3;
    for(;i<e;i+=4){
      {
        // mask is on for active rays
        __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0xffffffff));
        //int intmask = 15;
        _mm_store_ps((float*)&(data->normal[0][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[0][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[0][i])), _mm_set1_ps(center.x())), _mm_set1_ps(inv_radius)))));
        _mm_store_ps((float*)&(data->normal[1][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[1][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[1][i])), _mm_set1_ps(center.y())), _mm_set1_ps(inv_radius)))));
        _mm_store_ps((float*)&(data->normal[2][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[2][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[2][i])), _mm_set1_ps(center.z())), _mm_set1_ps(inv_radius)))));
        }
    }
    // Epilogue
    if(i != rays.rayEnd){
      __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
      // mask is on for active rays
      __m128 mask = _mm_castsi128_ps(_mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i)));
      //int intmask = _mm_movemask_ps(mask);
      _mm_store_ps((float*)&(data->normal[0][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[0][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[0][i])), _mm_set1_ps(center.x())), _mm_set1_ps(inv_radius)))));
      _mm_store_ps((float*)&(data->normal[1][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[1][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[1][i])), _mm_set1_ps(center.y())), _mm_set1_ps(inv_radius)))));
      _mm_store_ps((float*)&(data->normal[2][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[2][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[2][i])), _mm_set1_ps(center.z())), _mm_set1_ps(inv_radius)))));
    }
  } else {
  // Single SSE vector
    int i = rays.rayBegin & ~3;
    __m128i ray_idx = _mm_set_epi32(3, 2, 1, 0);
    __m128 mask = _mm_castsi128_ps(_mm_and_si128(_mm_cmpgt_epi32(ray_idx, _mm_set1_epi32(rays.rayBegin-i-1)), _mm_cmplt_epi32(ray_idx, _mm_set1_epi32(rays.rayEnd-i))));
    //int intmask = _mm_movemask_ps(mask);
    _mm_store_ps((float*)&(data->normal[0][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[0][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[0][i])), _mm_set1_ps(center.x())), _mm_set1_ps(inv_radius)))));
    _mm_store_ps((float*)&(data->normal[1][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[1][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[1][i])), _mm_set1_ps(center.y())), _mm_set1_ps(inv_radius)))));
    _mm_store_ps((float*)&(data->normal[2][i]), _mm_or_ps(_mm_andnot_ps(mask, _mm_load_ps((float*)&(data->normal[2][i]))), _mm_and_ps(mask, _mm_mul_ps(_mm_sub_ps(_mm_load_ps((float*)&(data->hitPosition[2][i])), _mm_set1_ps(center.z())), _mm_set1_ps(inv_radius)))));
  }
#else
  for(int i = rays.begin(); i < rays.end(); i++){
    {
        rays.setNormal(i, ((rays.getHitPosition(i) - center) * inv_radius));
      }
    }
#endif
}
